#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iterator>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "GPSProtocolTestIO.h"
#include "ProtocolTestPackets.h"
#include "Support/ScriptedReceiver.h"
#include "Support/UBXReceiverModel.h"
#include "UBX/GPSDriverUBX.h"
#include "UBX/UBXMessageCodec.h"
#include "UnitTest.h"

// Keep checks active in Release, too.
#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

namespace {
using Bytes = std::vector<uint8_t>;

using SurveyReply = UBXReceiverModel::SurveyReply;

class ProtocolReceiver final : public UBXReceiverModel
{
public:
    ProtocolReceiver()
        : UBXReceiverModel(UBXReceiverModel::Receiver::F9P)
        , _transport(_stop, *this)
    {
        _configure();
        _transport.setModel(this);
    }

    GPSProtocolIO io()
    {
        _configure();
        _transport.clearReplies();
        _transport.clearCommands();
        _transport.setWriteHandler([this](const QByteArray& bytes, const ScriptedReceiver::WriteContext&) {
            return interceptLowLevelWrite(bytes);
        });
        auto result = makeGPSProtocolTestIO();
        result.wait = [this](std::chrono::microseconds delay) {
            ++transportOperations;
            gps_test_time += delay.count();
            return true;
        };
        result.decoded = [this](const GPSDecodedBatch& batch) {
            for (const auto& event : batch.events) {
                if (const auto* report = std::get_if<GPSIntegrityReport>(&event)) {
                    integrity = *report;
                    ++integrityCount;
                }
            }
            for (const auto& event : batch.events) {
                if (std::holds_alternative<GPSNativeSurveyReport>(event)) {
                    ++statusCallbacks;
                }
            }
        };
        auto scripted = _transport.makeIO(std::move(result));
        auto read = std::move(scripted.read);
        scripted.read = [read = std::move(read)](std::span<uint8_t> bytes, GPSDeadline deadline) mutable {
            const auto readResult = read(bytes, deadline);
            if (readResult.status == GPSReadStatus::TimedOut) {
                gps_test_time = deadline.untilUs + 1;
            } else {
                gps_test_time += 1000;
            }
            return readResult;
        };
        return scripted;
    }

    void resetState()
    {
        UBXReceiverModel::operator=(UBXReceiverModel(UBXReceiverModel::Receiver::F9P));
        _configure();
        _transport.setModel(this);
    }

private:
    void _configure()
    {
        lowLevelProtocolBehavior = true;
        nowUs = [] { return gps_test_time; };
    }

    std::atomic_bool _stop{false};
    ScriptedReceiver _transport;
};

struct Fixture
{
    ProtocolReceiver receiver;
    GPSNativePositionReport position{};
    GPSNativeUBX driver;
    GPSBaseStationConfig base;

    explicit Fixture(bool satelliteInfo = false)
        : driver(captureGPSReports(receiver.io(), position), satelliteInfo)
    {
        gps_test_time = 0;
        gps_test_warnings.clear();
        std::get<GPSBaseStationConfig::SurveyIn>(base.mode).accuracyMeters = 1.25;
        std::get<GPSBaseStationConfig::SurveyIn>(base.mode).durationSecs = 60;
    }

    bool configure()
    {
        unsigned baudrate = 115200;
        GPSProtocol::GPSConfig config{};
        config.base = base;
        return driver.configure(baudrate, config);
    }

    void success(unsigned expectedSurveyPolls)
    {
        CHECK(configure());
        CHECK(driver.receiverReady());
        CHECK(receiver.modes == std::vector<uint32_t>({0, 1}));
        CHECK(receiver.surveyPolls == expectedSurveyPolls);
        CHECK(receiver.starts == 1);
        CHECK(receiver.startSettings.at(UBX_CFG_KEY_TMODE_SVIN_MIN_DUR) == 60);
        CHECK(receiver.startSettings.at(UBX_CFG_KEY_TMODE_SVIN_ACC_LIMIT) == 12500);
        CHECK(receiver.startSettings.at(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C + 1) == 5);
        CHECK(receiver.startSettings.at(UBX_CFG_KEY_MSGOUT_UBX_NAV_SVIN_I2C + 3) == 5);
        CHECK(receiver.statusCallbacks == 0);
        CHECK(receiver.rtcmEnables == 0);
    }

    void timeout()
    {
        CHECK(!configure());
        CHECK(!driver.receiverReady());
        CHECK(receiver.modes == std::vector<uint32_t>({0}));
        CHECK(receiver.starts == 0);
        CHECK(receiver.surveyPolls > 1 && receiver.surveyPolls <= 31);
        CHECK(gps_test_time - receiver.disabledAt >= 3000000);
        CHECK(gps_test_time - receiver.disabledAt < 3300000);
        CHECK(receiver.statusCallbacks == 0);
        CHECK(receiver.rtcmEnables == 0);
    }

    void readFailure(GPSProtocolError error)
    {
        receiver.pollReadError = error;
        CHECK(!configure());
        CHECK(!driver.receiverReady());
        CHECK(receiver.failedReads == 1);
        CHECK(receiver.surveyPolls == 1);
        CHECK(receiver.modes == std::vector<uint32_t>({0}));
        CHECK(receiver.starts == 0);
        CHECK(receiver.statusCallbacks == 0);
        CHECK(receiver.rtcmEnables == 0);
        CHECK(gps_test_time - receiver.disabledAt < 100000);

        const auto warnings = gps_test_warnings;
        if (error == GPSProtocolError::Cancelled) {
            CHECK(warnings.empty());
        } else {
            CHECK(warnings == QStringList{QStringLiteral("Receiver read failed (status %1): %2")
                                              .arg(static_cast<int>(GPSReadStatus::Error))
                                              .arg(receiver.pollReadDetail)});
        }
        CHECK(driver.ioError() == error);
        CHECK(driver.ioErrorDetail() == receiver.pollReadDetail);
        CHECK(driver.receive(10) == 0);
        CHECK(receiver.failedReads == 1);
        CHECK(gps_test_warnings == warnings);
    }
};

static void receiveFailureLogging()
{
    for (const GPSProtocolError error : {GPSProtocolError::Cancelled, GPSProtocolError::Transport}) {
        Fixture f;
        CHECK(f.configure());
        f.receiver.pollReadError = error;
        gps_test_warnings.clear();
        CHECK(f.driver.receive(10) == 0);
        CHECK(f.driver.ioError() == error);
        CHECK(f.driver.ioErrorDetail() == f.receiver.pollReadDetail);
        const auto warnings = gps_test_warnings;
        if (error == GPSProtocolError::Cancelled) {
            CHECK(warnings.empty());
        } else {
            CHECK(warnings == QStringList{QStringLiteral("Receiver read failed (status %1): %2")
                                              .arg(static_cast<int>(GPSReadStatus::Error))
                                              .arg(f.receiver.pollReadDetail)});
        }
        CHECK(f.driver.receive(10) == 0);
        CHECK(f.driver.ioError() == error);
        CHECK(f.receiver.failedReads == 1);
        CHECK(gps_test_warnings == warnings);
    }
}

static Bytes commsPayload()
{
    Bytes payload(88, 0);
    payload[1] = 2;
    payload[2] = 2;
    // USB: 11800 bytes pending, current usage 100%, historical peak 101%.
    payload[9] = 3;
    payload[10] = 0x18;
    payload[11] = 0x2e;
    payload[16] = 100;
    payload[17] = 101;
    payload[18] = 12;
    payload[24] = 3;
    payload[26] = 4;
    payload[44] = 0x40;
    payload[45] = 0xe2;
    payload[46] = 1;
    // UART2: no current congestion, despite a historical peak of 108%.
    payload[48] = 1;
    payload[49] = 2;
    payload[57] = 108;
    return payload;
}

static void integrityReceipts()
{
    Fixture f;
    CHECK(f.configure());
    // This test inspects individual decoder mutations; epoch assembly has separate coverage.
    f.driver.setDecodeContext({.navigation = true});
    Bytes mon_rf(UBX::WIRE_SIZE<ubx_payload_rx_mon_rf_t>, 0);
    mon_rf[1] = 1;
    mon_rf[5] = 3;
    f.receiver.queueBytes(ubxFrame(UBX_MSG_MON_RF, mon_rf));
    f.driver.receive(100);
    CHECK(f.receiver.integrityCount == 1);
    CHECK(f.position.navigation.timestampUs == 0);
    CHECK(f.receiver.integrity.jamming.state == GPSIntegrityReport::JammingState::Critical);
    const auto rf_stamp = f.receiver.integrity.jamming.timestampUs;
    CHECK(rf_stamp != 0);

    Bytes nav_status(UBX::WIRE_SIZE<ubx_payload_rx_nav_status_t>, 0);
    nav_status[7] = 1 << UBX_RX_NAV_STATUS_SPOOFDETSTATE_SHIFT;
    f.receiver.queueBytes(ubxFrame(UBX_MSG_NAV_STATUS, nav_status));
    f.driver.receive(100);
    const auto spoof_stamp = f.receiver.integrity.spoofing.timestampUs;
    CHECK(spoof_stamp != 0);
    CHECK(f.receiver.integrity.jamming.timestampUs == rf_stamp);

    Bytes pvt(UBX::WIRE_SIZE<ubx_payload_rx_nav_pvt_t>, 0);
    pvt[20] = 3;
    pvt[21] = 1;
    for (int i = 0; i < 10; ++i) {
        gps_test_time += 1000000;
        f.receiver.queueBytes(ubxFrame(UBX_MSG_NAV_PVT, pvt));
        CHECK(f.driver.receive(100) & 1);
        CHECK(f.position.navigation.timestampUs > rf_stamp);
        CHECK(f.receiver.integrity.jamming.timestampUs == rf_stamp);
        CHECK(f.receiver.integrity.spoofing.timestampUs == spoof_stamp);
    }
    Bytes corrupt = ubxFrame(UBX_MSG_MON_RF, mon_rf);
    corrupt.back() ^= 0xff;
    f.receiver.queueBytes(corrupt);
    f.driver.receive(100);
    CHECK(f.receiver.integrity.jamming.timestampUs == rf_stamp);
    f.receiver.queueBytes(ubxFrame(UBX_MSG_MON_RF, mon_rf));
    f.driver.receive(100);
    CHECK(f.receiver.integrity.jamming.state == GPSIntegrityReport::JammingState::Critical);
    CHECK(f.receiver.integrity.jamming.timestampUs > rf_stamp);

    Bytes sec_sig(4, 0);
    sec_sig[0] = 2;
    sec_sig[1] = 1 | (3 << 1);
    f.receiver.queueBytes(ubxFrame(UBX_MSG_SEC_SIG, sec_sig));
    f.driver.receive(100);
    CHECK(f.receiver.integrity.jamming.state == GPSIntegrityReport::JammingState::Critical);
    const auto sec_stamp = f.receiver.integrity.jamming.timestampUs;
    gps_test_time += 6000000;
    f.receiver.queueBytes(ubxFrame(UBX_MSG_NAV_PVT, pvt));
    CHECK(f.driver.receive(100) & 1);
    CHECK(f.receiver.integrity.jamming.timestampUs == sec_stamp);
    f.receiver.queueBytes(ubxFrame(UBX_MSG_SEC_SIG, sec_sig));
    f.driver.receive(100);
    CHECK(f.receiver.integrity.jamming.state == GPSIntegrityReport::JammingState::Critical);
    CHECK(f.receiver.integrity.jamming.timestampUs > sec_stamp);

    Bytes rtcm(UBX::WIRE_SIZE<ubx_payload_rx_rxm_rtcm_t>, 0);
    rtcm[1] = 2 << UBX_RX_RXM_RTCM_MSGUSED_SHIFT;
    f.receiver.queueBytes(ubxFrame(UBX_MSG_RXM_RTCM, rtcm));
    f.driver.receive(100);
    CHECK(f.receiver.integrity.corrections.use == GPSIntegrityReport::CorrectionUse::Used);
    const auto correction_stamp = f.receiver.integrity.corrections.timestampUs;
    CHECK(correction_stamp != 0);
    gps_test_time += 6000000;
    f.receiver.queueBytes(ubxFrame(UBX_MSG_NAV_PVT, pvt));
    CHECK(f.driver.receive(100) & 1);
    CHECK(f.receiver.integrity.corrections.timestampUs == correction_stamp);
    Bytes cor(UBX::WIRE_SIZE<ubx_payload_rx_rxm_cor_t>, 0);
    cor[0] = 1;
    cor[4] = 29;
    cor[5] = 1;  // msgUsed=2 in statusInfo bits 8..7.
    f.receiver.queueBytes(ubxFrame(UBX_MSG_RXM_COR, cor));
    f.driver.receive(100);
    CHECK(f.receiver.integrity.corrections.protocol == GPSIntegrityReport::CorrectionProtocol::PMP);
    CHECK(f.receiver.integrity.corrections.use == GPSIntegrityReport::CorrectionUse::Used);
    CHECK(f.receiver.integrity.corrections.timestampUs > correction_stamp);
}

static void commsDiagnostics()
{
    Fixture f;
    const Bytes reply = ubxFrame(UBX_MSG_MON_COMMS, commsPayload());
    f.receiver.queueBytes(reply);
    f.driver.receive(100);
    CHECK(gps_test_warnings.empty());
    CHECK(f.receiver.commsPolls == 0);
    f.receiver.queueBufferWarning();
    f.driver.receive(100);
    CHECK(f.receiver.commsPolls == 1);
    CHECK(gps_test_warnings == QStringList{"ubx msg: txbuf alloc"});
    gps_test_warnings.clear();
    f.receiver.queueBytes(reply);
    CHECK((f.driver.receive(100) & GPSDecodedBatch::POSITION_UPDATE) ==
          0);  // Diagnostic traffic alone is not a position update.
    const QStringList expected{"MON-COMMS after txbuf: txErrors=0x02 ports=2 (snapshot after warning)",
                               "MON-COMMS USB port=0x0300 txPending=11800 txUsage=100% txPeakUsage=101% "
                               "rxPending=12 rxUsage=3% overrunErrs=4 skipped=123456",
                               "MON-COMMS UART2 port=0x0201 txPending=0 txUsage=0% txPeakUsage=108% "
                               "rxPending=0 rxUsage=0% overrunErrs=0 skipped=0"};
    CHECK(gps_test_warnings == expected);
    gps_test_warnings.clear();
    f.receiver.queueBytes(reply);
    f.driver.receive(100);
    CHECK(gps_test_warnings.empty());
}

static void invalidCommsDiagnostics()
{
    Bytes payload = commsPayload();
    Bytes corrupt = ubxFrame(UBX_MSG_MON_COMMS, payload);
    corrupt.back() ^= 0xff;
    std::vector<Bytes> invalid{corrupt, ubxFrame(UBX_MSG_MON_COMMS, Bytes(7, 0)),
                               ubxFrame(UBX_MSG_MON_COMMS, Bytes(87, 0)), ubxFrame(UBX_MSG_MON_COMMS, Bytes(368, 0))};
    payload[0] = 1;
    invalid.push_back(ubxFrame(UBX_MSG_MON_COMMS, payload));
    payload[0] = 0;
    payload[1] = 3;
    invalid.push_back(ubxFrame(UBX_MSG_MON_COMMS, payload));
    payload[1] = 255;
    invalid.push_back(ubxFrame(UBX_MSG_MON_COMMS, payload));

    for (const auto& reply : invalid) {
        Fixture f;
        f.receiver.queueBufferWarning();
        f.driver.receive(100);
        gps_test_warnings.clear();
        f.receiver.queueBytes(reply);
        f.driver.receive(100);
        CHECK(gps_test_warnings.empty());
        // Malformed input must not consume the pending reply or lose framing.
        f.receiver.queueBytes(ubxFrame(UBX_MSG_MON_COMMS, Bytes(8, 0)));
        f.driver.receive(100);
        CHECK(gps_test_warnings ==
              QStringList{"MON-COMMS after txbuf: txErrors=0x00 ports=0 (snapshot after warning)"});
    }
}

static void expiredCommsDiagnostics()
{
    Fixture f;
    f.receiver.queueBufferWarning();
    f.driver.receive(100);
    CHECK(f.receiver.commsPolls == 1);
    gps_test_warnings.clear();
    gps_test_time += 2000000;
    f.receiver.queueBytes(ubxFrame(UBX_MSG_MON_COMMS, commsPayload()));
    f.driver.receive(100);
    CHECK(gps_test_warnings.empty());
    // Expiration must allow a later warning to obtain a fresh snapshot.
    gps_test_time += 5000000;
    f.receiver.queueBufferWarning();
    f.driver.receive(100);
    CHECK(f.receiver.commsPolls == 2);
    gps_test_warnings.clear();
    f.receiver.queueBytes(ubxFrame(UBX_MSG_MON_COMMS, Bytes(8, 0)));
    f.driver.receive(100);
    CHECK(gps_test_warnings.size() == 1);
}

static void invalidConfiguration()
{
    using Config = GPSProtocol::GPSConfig;
    const Config fixed{.base = {.mode = GPSBaseStationConfig::Fixed{
                                    .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
                                    .accuracyMeters = 1}}};
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto infinity = std::numeric_limits<float>::infinity();
    std::vector<Config> invalid;
    const auto addFixed = [&](auto mutate) {
        auto config = fixed;
        mutate(config.base);
        invalid.push_back(config);
    };
    addFixed([](auto& base) { base = {.mode = GPSBaseStationConfig::Fixed{}}; });
    for (double value : {nan, double(infinity), 91.0, -91.0}) {
        addFixed(
            [&](auto& base) { std::get<GPSBaseStationConfig::Fixed>(base.mode).position.latitudeDegrees = value; });
    }
    for (double value : {nan, double(infinity), 181.0, -181.0}) {
        addFixed(
            [&](auto& base) { std::get<GPSBaseStationConfig::Fixed>(base.mode).position.longitudeDegrees = value; });
    }
    for (float value : {float(nan), infinity, 21474838.0f, -21474838.0f}) {
        addFixed([&](auto& base) { std::get<GPSBaseStationConfig::Fixed>(base.mode).position.altitudeMeters = value; });
    }
    for (float value : {float(nan), infinity, -1.0f, std::nextafter(429496.71875f, infinity)}) {
        addFixed([&](auto& base) { std::get<GPSBaseStationConfig::Fixed>(base.mode).accuracyMeters = value; });
    }
    for (double accuracy : {nan, double(infinity), 0.0, -1.0, 429496.7296}) {
        invalid.push_back(
            {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = accuracy, .durationSecs = 60}}});
    }
    for (int64_t duration : {int64_t(0), int64_t(-1), int64_t(UINT32_MAX) + 1}) {
        invalid.push_back(
            {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .durationSecs = duration}}});
    }
    for (const auto& config : invalid) {
        for (bool wasReady : {false, true}) {
            Fixture f;
            if (wasReady) {
                CHECK(f.configure());
                CHECK(f.driver.receiverReady());
            }
            f.receiver.transportOperations = 0;
            gps_test_warnings.clear();
            unsigned baudrate = 115200;
            CHECK(!f.driver.configure(baudrate, config));
            CHECK(f.receiver.transportOperations == 0);
            CHECK(!f.driver.receiverReady());
            CHECK(!f.driver.hasIOError());
            CHECK(baudrate == 115200);
            CHECK(gps_test_warnings.size() == 1);
        }
    }

    for (bool legacy : {false, true}) {
        Fixture f;
        f.receiver.legacy = legacy;
        f.receiver.module = legacy ? "NEO-M8P" : "ZED-F9P";
        f.base = fixed.base;
        std::get<GPSBaseStationConfig::Fixed>(f.base.mode).accuracyMeters = 429496.71875f;
        CHECK(f.configure());
        CHECK(f.driver.receiverReady());
        CHECK((legacy ? f.receiver.legacyFixedAccuracy
                      : f.receiver.currentSettings.at(UBX_CFG_KEY_TMODE_FIXED_POS_ACC)) == 4294967040u);
    }

    // Compact bases send MSM4 and switch off MSM7 left over from an earlier session, and vice versa.
    constexpr std::array<std::pair<uint32_t, uint16_t>, 4> msm7{{
        {UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1077_I2C, UBX_MSG_RTCM3_1077},
        {UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1087_I2C, UBX_MSG_RTCM3_1087},
        {UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1097_I2C, UBX_MSG_RTCM3_1097},
        {UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1127_I2C, UBX_MSG_RTCM3_1127},
    }};
    constexpr std::array<std::pair<uint32_t, uint16_t>, 4> msm4{{
        {UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1074_I2C, UBX_MSG_RTCM3_1074},
        {UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1084_I2C, UBX_MSG_RTCM3_1084},
        {UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1094_I2C, UBX_MSG_RTCM3_1094},
        {UBX_CFG_KEY_MSGOUT_RTCM_3X_TYPE1124_I2C, UBX_MSG_RTCM3_1124},
    }};
    for (bool legacy : {false, true}) {
        for (bool compact : {false, true}) {
            Fixture f(true);
            f.receiver.legacy = legacy;
            f.receiver.module = legacy ? "NEO-M8P" : "ZED-F9P";
            f.base = fixed.base;
            f.base.compactObservations = compact;
            CHECK(f.configure());
            CHECK(f.driver.receiverReady());
            const auto rate = [&](const std::pair<uint32_t, uint16_t>& message) -> unsigned {
                // UART1 follows the I2C key.
                return legacy ? f.receiver.messageRates.at(message.second)
                              : f.receiver.currentSettings.at(message.first + 1);
            };
            for (const auto& message : msm7) {
                CHECK(rate(message) == (compact ? 0u : 1u));
            }
            for (const auto& message : msm4) {
                CHECK(rate(message) == (compact ? 1u : 0u));
            }
            // The 1 Hz base rate keeps satellite reports within their freshness window.
            CHECK(rate({UBX_CFG_KEY_MSGOUT_UBX_NAV_SAT_I2C, UBX_MSG_NAV_SVINFO}) == 2u);
        }
    }
}

static void explicitNoFix()
{
    ProtocolReceiver receiver;
    GPSProtocolTestProbe<GPSNativeUBX> driver(receiver.io(), false);
    const auto& position = driver.workingPosition();
    CHECK(position.navigation.fixType == GPSPositionReport::FixType::Unknown);
    driver.setDecodeContext({.navigation = true});
    Bytes payload(UBX::WIRE_SIZE<ubx_payload_rx_nav_pvt_t>, 0);
    payload[20] = 3;
    for (const uint8_t flags : std::array<uint8_t, 4>{0, 2, 0x40, 0x80}) {
        payload[21] = UBX_RX_NAV_PVT_FLAGS_GNSSFIXOK;
        const auto valid = driver.decode(ubxFrame(UBX_MSG_NAV_PVT, payload));
        CHECK(valid.batch.events.size() == 1);
        CHECK(std::get<GPSNativePositionReport>(valid.batch.events.front()).navigation.fixType ==
              GPSPositionReport::FixType::Fix3D);
        CHECK(position.navigation.fixType == GPSPositionReport::FixType::Fix3D);
        payload[21] = flags;
        const auto decoded = driver.decode(ubxFrame(UBX_MSG_NAV_PVT, payload));
        CHECK(decoded.batch.events.size() == 1);
        CHECK(std::get<GPSNativePositionReport>(decoded.batch.events.front()).navigation.fixType ==
              GPSPositionReport::FixType::NoFix);
        CHECK(!std::get<GPSNativePositionReport>(decoded.batch.events.front()).velocityValid);
        CHECK(!position.velocityValid);
    }
}

static void outOfRangeCoordinates()
{
    ProtocolReceiver receiver;
    GPSProtocolTestProbe<GPSNativeUBX> driver(receiver.io(), false);
    const auto& position = driver.workingPosition();
    driver.setDecodeContext({.navigation = true});
    Bytes pvt(UBX::WIRE_SIZE<ubx_payload_rx_nav_pvt_t>, 0);
    pvt[20] = 3;
    pvt[21] = UBX_RX_NAV_PVT_FLAGS_GNSSFIXOK;
    const auto decodePvt = [&](int32_t longitude, int32_t latitude) {
        (void) LittleEndian::write<int32_t>(pvt, 24, longitude);
        (void) LittleEndian::write<int32_t>(pvt, 28, latitude);
        (void) driver.decode(ubxFrame(UBX_MSG_NAV_PVT, pvt));
    };
    decodePvt(-1800000000, 900000000);
    CHECK(position.navigation.latitudeDegrees == 90 && position.navigation.longitudeDegrees == -180);
    decodePvt(80000000, 900000001);
    CHECK(std::isnan(position.navigation.latitudeDegrees));
    CHECK(position.navigation.longitudeDegrees == 8);
    decodePvt(-1800000001, 470000000);
    CHECK(position.navigation.latitudeDegrees == 47);
    CHECK(std::isnan(position.navigation.longitudeDegrees));

    driver.setDecodeContext({.navigation = true, .useNavPvt = false});
    Bytes posllh(UBX::WIRE_SIZE<ubx_payload_rx_nav_posllh_t>, 0);
    (void) LittleEndian::write<int32_t>(posllh, 4, (std::numeric_limits<int32_t>::max)());
    (void) LittleEndian::write<int32_t>(posllh, 8, (std::numeric_limits<int32_t>::min)());
    (void) driver.decode(ubxFrame(UBX_MSG_NAV_POSLLH, posllh));
    CHECK(std::isnan(position.navigation.latitudeDegrees));
    CHECK(std::isnan(position.navigation.longitudeDegrees));
}

static void baudDiscovery()
{
    for (const unsigned initialBaud : {9600U, 115200U}) {
        for (const bool fixed : {false, true}) {
            for (const bool usb : {false, true}) {
                for (const bool loseAck : {false, true}) {
                    gps_test_time = 1000000;
                    ProtocolReceiver receiver;
                    receiver.receiverBaud = initialBaud;
                    receiver.usb = usb;
                    receiver.loseBaudAck = loseAck;
                    receiver.protocol = "27.31";
                    GPSNativePositionReport position;
                    GPSNativeUBX driver(captureGPSReports(receiver.io(), position), false);
                    GPSProtocol::GPSConfig config{};
                    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
                    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).durationSecs = 60;
                    unsigned baud = fixed ? initialBaud : 0;
                    CHECK(driver.configure(baud, config));
                    CHECK(driver.receiverReady());
                    CHECK(receiver.unidentifiedWrites == 0);
                    CHECK(baud == (fixed ? initialBaud : 115200));
                    CHECK(receiver.receiverBaud == baud);
                    CHECK(receiver.hostBaud == baud);
                    CHECK(receiver.currentSettings.at(UBX_CFG_KEY_NAVSPG_DYNMODEL) == 2);
                    CHECK(receiver.currentSettings.at(UBX_CFG_KEY_RATE_MEAS) == 200);
                    if (fixed) {
                        CHECK(receiver.hostBauds == std::vector<unsigned>{initialBaud});
                    } else if (!usb) {
                        const std::vector<unsigned> probes = initialBaud == 9600
                                                                 ? std::vector<unsigned>{38400, 57600, 9600}
                                                                 : std::vector<unsigned>{38400, 57600, 9600, 115200};
                        CHECK(receiver.identityBauds.size() >= probes.size());
                        CHECK(std::equal(probes.begin(), probes.end(), receiver.identityBauds.begin()));
                    }
                    CHECK(gps_test_time < 15000000);
                }
            }
        }
    }
}

static void discoveryFailures()
{
    for (unsigned scenario = 0; scenario < 10; ++scenario) {
        gps_test_time = 1000000;
        ProtocolReceiver receiver;
        receiver.receiverBaud = 9600;
        receiver.hardware = scenario == 0 ? "UNKN0WN!" : "00190000";
        receiver.protocol = scenario == 1 ? "invalid" : "27.31";
        receiver.corruptIdentity = scenario == 2;
        receiver.silencePortConfiguration = scenario == 3;
        receiver.loseBaudAck = scenario >= 4;
        receiver.ignoreBaudChange = scenario == 4 || scenario == 5;
        receiver.usb = scenario == 5;
        receiver.rejectAfterBaudChange = scenario == 6;
        receiver.nakAfterBaudChange = scenario == 8;
        receiver.readbackReply =
            scenario == 9 ? UBXReceiverModel::ReadbackReply::Timeout : UBXReceiverModel::ReadbackReply::Value;
        receiver.module = scenario == 7 ? "NEO-M9N" : "ZED-F9P";
        GPSNativePositionReport position;
        GPSNativeUBX driver(captureGPSReports(receiver.io(), position), false);
        GPSProtocol::GPSConfig config{};
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).durationSecs = 60;
        unsigned baud = 0;
        CHECK(!driver.configure(baud, config));
        CHECK(!driver.receiverReady());
        CHECK(receiver.unidentifiedWrites == 0);
        if (scenario < 3 || scenario == 7) {
            CHECK(receiver.configurationWrites == 0);
        } else if (scenario == 3) {
            CHECK(receiver.configurationWrites == 1);
        } else if (scenario == 6) {
            CHECK(receiver.lateBaudAckDelivered);
            CHECK(receiver.currentSettings.count(UBX_CFG_KEY_CFG_USBOUTPROT_UBX) == 0);
        } else if (scenario == 8) {
            CHECK(receiver.lateBaudAckDelivered);
            CHECK(receiver.currentSettings.at(UBX_CFG_KEY_CFG_USBOUTPROT_UBX) == 1);
        }
        CHECK(gps_test_time < 20000000);
    }
}

static void navigationFixFlags()
{
    using Fix = GPSPositionReport::FixType;
    constexpr std::array<Fix, 8> FIX_3D{Fix::Fix3D,    Fix::Differential, Fix::RTKFloat, Fix::RTKFloat,
                                        Fix::RTKFixed, Fix::RTKFixed,     Fix::Fix3D,    Fix::Differential};
    constexpr std::array<Fix, 6> UNCORRECTED{Fix::NoFix, Fix::Extrapolated, Fix::Fix2D,
                                             Fix::Fix3D, Fix::Fix3D,        Fix::NoFix};
    for (const bool legacy : {false, true}) {
        for (const unsigned rawFix : {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 255U}) {
            for (unsigned flags = 0; flags <= UINT8_MAX; ++flags) {
                Fix expected = rawFix < UNCORRECTED.size() ? UNCORRECTED[rawFix] : Fix::Unknown;
                if (!(flags & 1)) {
                    expected = Fix::NoFix;
                } else if (rawFix == 3 || rawFix == 4) {
                    expected = legacy ? FIX_3D[(flags >> 1) & 1] : FIX_3D[((flags >> 6) * 2) + ((flags >> 1) & 1)];
                }
                std::array<unsigned, 3> order{0, 1, 2};
                do {
                    gps_test_time = 1000000;
                    GPSNativePositionReport position;
                    GPSNativeUBX driver(captureGPSReports(makeGPSProtocolTestIO(), position), false);
                    driver.setDecodeContext({.navigation = true, .useNavPvt = !legacy, .assembleEpochs = true});
                    Bytes pvt(92);
                    (void) LittleEndian::write<uint32_t>(pvt, 0, 1000);
                    pvt[20] = static_cast<uint8_t>(rawFix);
                    pvt[21] = static_cast<uint8_t>(flags);
                    (void) LittleEndian::write<int32_t>(pvt, 24, 80000000);
                    (void) LittleEndian::write<int32_t>(pvt, 28, 470000000);
                    (void) LittleEndian::write<int32_t>(pvt, 60, 12000);
                    if (!legacy) {
                        CHECK(driver.decode(ubxFrame(UBX_MSG_NAV_PVT, pvt)).batch.events.empty());
                    } else {
                        std::array<Bytes, 3> payloads{Bytes(28), Bytes(52), Bytes(36)};
                        constexpr std::array<uint16_t, 3> MESSAGES{UBX_MSG_NAV_POSLLH, UBX_MSG_NAV_SOL,
                                                                   UBX_MSG_NAV_VELNED};
                        for (auto& payload : payloads) {
                            (void) LittleEndian::write<uint32_t>(payload, 0, 1000);
                        }
                        (void) LittleEndian::write<int32_t>(payloads[0], 4, 80000000);
                        (void) LittleEndian::write<int32_t>(payloads[0], 8, 470000000);
                        payloads[1][10] = static_cast<uint8_t>(rawFix);
                        payloads[1][11] = static_cast<uint8_t>(flags);
                        (void) LittleEndian::write<uint32_t>(payloads[2], 20, 1200);
                        for (const unsigned index : order) {
                            CHECK(driver.decode(ubxFrame(MESSAGES[index], payloads[index])).batch.events.empty());
                        }
                    }
                    Bytes end(4);
                    (void) LittleEndian::write<uint32_t>(end, 0, 1000);
                    const auto decoded = driver.decode(ubxFrame(UBX::NAV_EOE, end));
                    CHECK(decoded.batch.events.size() == 1);
                    const auto& fix = std::get<GPSNativePositionReport>(decoded.batch.events.front());
                    CHECK(fix.navigation.fixType == expected);
                    CHECK(fix.velocityValid == (expected != Fix::NoFix && expected != Fix::Unknown));
                    CHECK(fix.navigation.latitudeDegrees == 47 && fix.navigation.longitudeDegrees == 8);
                    CHECK(std::abs(fix.navigation.speedMetersPerSecond - 12) < 1e-5f);
                } while (legacy && std::next_permutation(order.begin(), order.end()));
            }
        }
    }
}

static void transactionalFrames()
{
    ProtocolReceiver receiver;
    GPSProtocolTestProbe<GPSNativeUBX> driver(receiver.io());
    const auto& satellites = driver.workingSatellites();
    unsigned baud = 115200;
    GPSProtocol::GPSConfig config{};
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).durationSecs = 60;
    CHECK(driver.configure(baud, config));
    Bytes payload(20, 0);
    payload[4] = 1;
    payload[5] = 1;
    payload[8] = 0;
    payload[9] = 17;
    payload[10] = 35;
    const auto valid = ubxFrame(UBX_MSG_NAV_SAT, payload);
    payload[9] = 23;
    auto corrupt = ubxFrame(UBX_MSG_NAV_SAT, payload);
    corrupt.back() ^= 1;
    Bytes joined = valid;
    joined.insert(joined.end(), corrupt.begin(), corrupt.end());
    auto decoded = driver.decode(joined);
    CHECK(decoded.bytesConsumed == joined.size());
    CHECK(decoded.batch.events.size() == 1);
    CHECK(std::get<GPSNativeSatelliteReport>(decoded.batch.events[0]).constellations[0].inView == 1);
    CHECK(satellites.constellations[0].inView == 1);
    CHECK(driver.decode(std::span(valid).first(valid.size() - 1)).batch.events.empty());
    CHECK(satellites.constellations[0].inView == 1);
    CHECK(driver.decode(std::span(valid).last(1)).batch.events.size() == 1);
    payload[5] = 2;  // A valid checksum cannot make an incomplete counted payload valid.
    CHECK(driver.decode(ubxFrame(UBX_MSG_NAV_SAT, payload)).batch.events.empty());
    CHECK(satellites.constellations[0].inView == 1);
    CHECK(driver.decode(ubxFrame(UBX_MSG_NAV_SAT, Bytes(7, 0))).batch.events.empty());
    CHECK(satellites.constellations[0].inView == 1);
    CHECK(std::get<GPSNativeSatelliteReport>(decoded.batch.events[0]).constellations[0].inView == 1);
    const auto empty = driver.decode(ubxFrame(UBX_MSG_NAV_SAT, Bytes{0, 0, 0, 0, 1, 0, 0, 0}));
    CHECK(empty.batch.events.size() == 1);
    CHECK(std::get<GPSNativeSatelliteReport>(empty.batch.events.front()).constellations[0].inView == 0);
    CHECK(satellites.constellations[0].inView == 0);
    GPSNativeUBX withoutSatellites(receiver.io(), false);
    withoutSatellites.setDecodeContext({.navigation = true});
    CHECK(withoutSatellites.decode(valid).batch.events.empty());
    const std::string originalModel = driver.modelName();
    Bytes version(70, 0);
    const std::string module = "MOD=NEO-M9N";
    std::copy(module.begin(), module.end(), version.begin() + 40);
    auto badVersion = ubxFrame(UBX_MSG_MON_VER, version);
    badVersion.back() ^= 1;
    CHECK(driver.decode(badVersion).batch.events.empty());
    CHECK(driver.modelName() == originalModel);

    Bytes baseVersion(40, 0);
    const std::string firmware = "SPG 4.04";
    std::copy(firmware.begin(), firmware.end(), baseVersion.begin());
    CHECK(driver.decode(ubxFrame(UBX_MSG_MON_VER, baseVersion)).batch.events.empty());
    CHECK(driver.firmwareVersion() == firmware);
    const std::string model = driver.modelName();
    CHECK(driver.receiverIdentity() == (model.empty() ? firmware : model + ' ' + firmware));

    Bytes pvt(UBX::WIRE_SIZE<ubx_payload_rx_nav_pvt_t>, 0);
    pvt[20] = 3;
    pvt[21] = 1;
    driver.setDecodeContext({.navigation = true});
    Bytes epochs;
    for (uint8_t index = 1; index <= 20; ++index) {
        pvt[23] = index;
        const auto frame = ubxFrame(UBX_MSG_NAV_PVT, pvt);
        epochs.insert(epochs.end(), frame.begin(), frame.end());
    }
    size_t offset = 0;
    unsigned count = 0;
    while (offset < epochs.size()) {
        auto batch = driver.decode(std::span(epochs).subspan(offset));
        CHECK(batch.bytesConsumed > 0);
        CHECK(batch.batch.events.size() <= GPSDecodedBatch::MAX_EVENTS);
        offset += batch.bytesConsumed;
        for (const auto& event : batch.batch.events) {
            CHECK(std::get<GPSNativePositionReport>(event).navigation.satellitesUsed == ++count);
        }
    }
    CHECK(count == 20);
    driver.setDecodeContext({true, true, true});
    Bytes correction{0xd3, 0, 2, 0x3e, 0xd0};
    const auto crc = RTCMFramer::crc24q(correction);
    correction.insert(correction.end(), {uint8_t(crc >> 16), uint8_t(crc >> 8), uint8_t(crc)});
    std::copy(correction.begin(), correction.end(), pvt.begin() + 40);
    const auto embedded = driver.decode(ubxFrame(UBX_MSG_NAV_PVT, pvt));
    CHECK(embedded.batch.events.size() == 1);
    CHECK(std::holds_alternative<GPSNativePositionReport>(embedded.batch.events.front()));
    const auto standalone = driver.decode(correction);
    CHECK(standalone.batch.events.size() == 1);
    CHECK(std::holds_alternative<GPSRTCMReport>(standalone.batch.events.front()));
    std::vector<std::vector<uint8_t>> recovered;
    auto io = makeGPSProtocolTestIO();
    io.decoded = [&](const GPSDecodedBatch& batch) {
        CHECK(batch.events.size() <= GPSDecodedBatch::MAX_EVENTS);
        for (const auto& event : batch.events) {
            const auto& report = std::get<GPSRTCMReport>(event);
            recovered.emplace_back(report.bytes.begin(), report.bytes.begin() + report.size);
        }
    };
    GPSNativeUBX recoveryDriver(io, false);
    recoveryDriver.setDecodeContext({.corrections = true});
    verifyRTCMRecovery(recoveryDriver, recovered);
}

static void controlDeadline()
{
    ProtocolReceiver receiver;
    GPSNativePositionReport position{};
    gps_test_time = 1000000;
    auto io = receiver.io();
    const auto read = io.read;
    const auto write = io.write;
    bool expireRead = false;
    bool verifyWrite = false;
    io.read = [&](auto bytes, GPSDeadline deadline) {
        const auto result = read(bytes, deadline);
        if (expireRead && result.status == GPSReadStatus::Data) {
            gps_test_time = deadline.untilUs;
            expireRead = false;
            verifyWrite = true;
        }
        return result;
    };
    io.write = [&](auto bytes, GPSDeadline deadline) {
        if (verifyWrite) {
            CHECK(deadline.remainingMilliseconds(gps_test_time) > 0);
        }
        return write(bytes, deadline);
    };
    GPSNativeUBX driver(captureGPSReports(io, position), false);
    unsigned baud = 115200;
    GPSProtocol::GPSConfig config{};
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).durationSecs = 60;
    CHECK(driver.configure(baud, config));
    driver.receive(10);  // Drain the configuration responses before the timed warning.
    receiver.readChunk = GPS_READ_BUFFER_SIZE;
    receiver.queueBufferWarning();
    expireRead = true;
    driver.receive(10);
    CHECK(verifyWrite);
    CHECK(receiver.commsPolls == 1);
    CHECK(!driver.hasIOError());
}

static void identificationWriteBudget()
{
    for (bool expireWrite : {false, true}) {
        gps_test_time = 1000000;
        std::vector<uint64_t> writeDeadlines;
        std::vector<GPSCommandResult> completions;
        uint64_t replyDeadline = 0;
        auto io = makeGPSProtocolTestIO();
        io.read = [&](std::span<uint8_t>, GPSDeadline deadline) {
            if (!writeDeadlines.empty()) {
                replyDeadline = deadline.untilUs;
            }
            gps_test_time = deadline.untilUs;
            return GPSReadResult{GPSReadStatus::TimedOut};
        };
        io.write = [&](std::span<const uint8_t> bytes, GPSDeadline deadline) {
            if (gps_test_time >= deadline.untilUs) {
                return GPSWriteResult{GPSWriteStatus::TimedOut};
            }
            CHECK(deadline.remainingMilliseconds(gps_test_time) <= 250);
            writeDeadlines.push_back(deadline.untilUs);
            gps_test_time = expireWrite ? deadline.untilUs : gps_test_time + 40000;
            return GPSWriteResult{GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
        };
        io.commandFinished = [&](const GPSCommandResult& result) { completions.push_back(result); };
        GPSNativeUBX driver(std::move(io), false);
        unsigned baud = 115200;
        GPSProtocol::GPSConfig config;
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).durationSecs = 60;
        CHECK(!driver.configure(baud, config));
        CHECK(completions.size() == 1);
        const auto& evidence = completions.front().evidence;
        CHECK(evidence.command == std::to_string(UBX_MSG_MON_VER));
        CHECK(evidence.startedAtUs == 1020000);
        CHECK(writeDeadlines == std::vector<uint64_t>(expireWrite ? 1 : 2, evidence.startedAtUs + 250000));
        CHECK(evidence.acceptedBytes == (expireWrite ? 6 : 8));
        CHECK(evidence.writtenBytes == evidence.acceptedBytes && evidence.uncertainBytes == 0);
        CHECK(evidence.outcome == (expireWrite ? GPSCommandOutcome::TransportError : GPSCommandOutcome::TimedOut));
        CHECK(replyDeadline == (expireWrite ? 0 : evidence.startedAtUs + 2000000));
        CHECK(evidence.finishedAtUs == evidence.startedAtUs + (expireWrite ? 250000 : 2000000));
    }
}

static void reentrantPayload()
{
    ProtocolReceiver receiver;
    GPSNativePositionReport position{};
    auto io = receiver.io();
    GPSNativeUBX* active = nullptr;
    bool reentered = false;
    QStringList warnings;
    Bytes correction{0xd3, 0, 2, 0x3e, 0xd0};
    const auto crc = RTCMFramer::crc24q(correction);
    correction.insert(correction.end(), {uint8_t(crc >> 16), uint8_t(crc >> 8), uint8_t(crc)});
    io.log = [&](const QLoggingCategory&, GPSProtocolLogLevel, QStringView message) {
        warnings.push_back(message.toString());
        if (!reentered && message == u"ubx msg: txbuf alloc") {
            reentered = true;
            active->setDecodeContext({.navigation = true, .corrections = true});
            for (auto byte : ubxFrame(UBX_MSG_INF_WARNING, Bytes{'o', 'k'})) {
                active->decodeByte(byte);
            }
            for (auto byte : std::span(correction).first(4)) {
                active->decodeByte(byte);
            }
        }
    };
    GPSNativeUBX driver(captureGPSReports(std::move(io), position), false);
    active = &driver;
    driver.setDecodeContext({.navigation = true, .corrections = true});
    const std::string warning = "txbuf alloc";
    CHECK(driver.decode(ubxFrame(UBX_MSG_INF_WARNING, Bytes(warning.begin(), warning.end()))).batch.events.empty());
    CHECK(reentered);
    CHECK(warnings == (QStringList{"ubx msg: txbuf alloc", "ubx msg: ok"}));
    const auto decoded = driver.decode(std::span(correction).subspan(4));
    CHECK(decoded.batch.events.size() == 1);
    CHECK(std::holds_alternative<GPSRTCMReport>(decoded.batch.events.front()));
    CHECK(receiver.transportOperations == 0);
    driver.receive(1);
    CHECK(receiver.commsPolls == 1);
}

static void isolatedFrameAndControl()
{
    UBX::FrameDecoder decoder;
    const Bytes payload = {0x06, 0x24};
    const auto bytes = ubxFrame(UBX_MSG_ACK_ACK, payload);
    std::optional<UBX::Frame> frame;
    for (size_t index = 0; index < bytes.size(); ++index) {
        frame = decoder.consume(bytes[index]);
        CHECK(frame.has_value() == (index + 1 == bytes.size()));
    }
    CHECK(frame->message == UBX_MSG_ACK_ACK && frame->length == 2);
    CHECK(frame->payload[0] == 6 && frame->payload[1] == 0x24);
    const auto retained = *frame;
    auto corrupt = bytes;
    corrupt.back() ^= 1;
    for (auto byte : corrupt) {
        CHECK(!decoder.consume(byte));
    }
    CHECK(decoder.idle());
    CHECK(retained.payload[0] == 6);
    const auto longFrame = ubxFrame(UBX_MSG_INF_NOTICE, Bytes(4096, 0xa5));
    for (auto byte : longFrame) {
        frame = decoder.consume(byte);
    }
    CHECK(frame && frame->length == 4096 && frame->payload.back() == 0xa5);
    decoder.reset();
    for (auto byte : bytes) {
        decoder.consume(byte);
    }
    CHECK(frame->length == 4096 && frame->payload.front() == 0xa5);
    CHECK(retained.payload[0] == 6 && retained.payload[1] == 0x24);
    for (auto byte : std::span(bytes).first(5)) {
        CHECK(!decoder.consume(byte));
    }
    decoder.reset();
    for (auto byte : bytes) {
        frame = decoder.consume(byte);
    }
    CHECK(frame && frame->message == UBX_MSG_ACK_ACK && frame->length == 2);

    Bytes pvt(UBX::WIRE_SIZE<ubx_payload_rx_nav_pvt_t>, 0);
    pvt[20] = 3;
    pvt[21] = UBX_RX_NAV_PVT_FLAGS_GNSSFIXOK;
    const auto validUbx = ubxFrame(UBX_MSG_NAV_PVT, pvt);
    const auto validRtcm = rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0});
    Bytes stream{0xb5, 0x62, 0x01, 0x07, 0x01, 0x10};
    stream.insert(stream.end(), validUbx.begin(), validUbx.end());
    stream.insert(stream.end(), validRtcm.begin(), validRtcm.end());
    GPSNativeUBX receiver(makeGPSProtocolTestIO(), false);
    receiver.setDecodeContext({.navigation = true, .corrections = true});
    const auto recovered = receiver.decode(stream);
    CHECK(recovered.bytesConsumed == stream.size());
    CHECK(recovered.batch.events.size() == 2);
    CHECK(std::holds_alternative<GPSNativePositionReport>(recovered.batch.events[0]));
    CHECK(std::holds_alternative<GPSRTCMReport>(recovered.batch.events[1]));
    CHECK(!receiver.hasIOError());

    for (size_t prefix = 1; prefix <= 4; ++prefix) {
        decoder.reset();
        for (size_t i = 0; i < prefix; ++i) {
            CHECK(!decoder.consume(0xb5));
        }
        unsigned completed = 0;
        for (auto byte : bytes) {
            if (const auto overlapping = decoder.consume(byte)) {
                ++completed;
                CHECK(overlapping->message == UBX_MSG_ACK_ACK);
                CHECK(overlapping->length == payload.size());
                CHECK(std::equal(payload.begin(), payload.end(), overlapping->payload.begin()));
            }
        }
        CHECK(completed == 1);
        CHECK(decoder.idle());
    }

    UBX::ReceiverController controller;
    controller.beginAcknowledgement(UBX_MSG_CFG_NAV5);
    controller.accept(UBX::Acknowledgement{UBX_MSG_CFG_RATE, true});
    CHECK(controller.acknowledgement() == GPSCommandOutcome::Pending);
    controller.accept(UBX::Acknowledgement{UBX_MSG_CFG_NAV5, false});
    CHECK(controller.acknowledgement() == GPSCommandOutcome::Rejected);
    controller.finishAcknowledgement();
    controller.accept(UBX::Acknowledgement{UBX_MSG_CFG_NAV5, true});
    CHECK(controller.acknowledgement() == GPSCommandOutcome::Rejected);

    const std::array<uint32_t, 2> keys{UBX_CFG_KEY_NAVSPG_DYNMODEL, UBX_CFG_KEY_RATE_MEAS};
    controller.beginReadback(keys);
    UBX::ConfigurationValues values;
    values.count = 2;
    values.values[0].key = values.values[1].key = keys[0];
    controller.accept(values);
    CHECK(!controller.readbackReady());
    values.values[0] = {.key = keys[1], .value = 200};
    values.values[1] = {.key = keys[0], .value = 4};
    controller.accept(values);
    CHECK(controller.readbackReady());
    CHECK(controller.readback().values[0].value == 4 && controller.readback().values[1].value == 200);
    CHECK(!UBX::decodeConfigurationValues(Bytes{1, 0, 0, 0, 1}));
}

static void checkedWireCodecs()
{
    const auto fixed = []<typename T>() {
        using Codec = UBX::MessageCodec<T>;
        Bytes payload(UBX::WIRE_SIZE<T>, 0);
        CHECK(Codec::decode(payload));
        CHECK(!Codec::decode({}));
        payload.pop_back();
        CHECK(!Codec::decode(payload));
        payload.resize(UBX::WIRE_SIZE<T> + 1);
        CHECK(!Codec::decode(payload));
    };
    fixed.operator()<ubx_payload_rx_nav_posllh_t>();
    fixed.operator()<ubx_payload_rx_nav_dop_t>();
    fixed.operator()<ubx_payload_rx_nav_sol_t>();
    fixed.operator()<ubx_payload_rx_nav_pvt_t>();
    fixed.operator()<ubx_payload_rx_nav_timeutc_t>();
    fixed.operator()<ubx_payload_rx_nav_status_t>();
    fixed.operator()<ubx_payload_rx_nav_svin_t>();
    fixed.operator()<ubx_payload_rx_nav_velned_t>();
    fixed.operator()<ubx_payload_rx_nav_relposned_t>();
    fixed.operator()<ubx_payload_rx_nav_daheading_t>();
    fixed.operator()<ubx_payload_rx_nav_hpposllh_t>();
    fixed.operator()<ubx_payload_rx_ack_ack_t>();
    fixed.operator()<ubx_payload_rx_ack_nak_t>();
    fixed.operator()<ubx_payload_rx_rxm_rtcm_t>();
    fixed.operator()<ubx_payload_rx_mon_hw_ubx6_t>();
    fixed.operator()<ubx_payload_rx_mon_hw_ubx7_t>();
    const auto hardware = []<typename T>(size_t jammingOffset) {
        Bytes payload(UBX::WIRE_SIZE<T>, 0xa5);
        CHECK(LittleEndian::write<uint16_t>(payload, 16, 0x1234));
        CHECK(LittleEndian::write<uint16_t>(payload, 18, 0x5678));
        payload[jammingOffset] = 77;
        const auto decoded = UBX::MessageCodec<T>::decode(payload);
        CHECK(decoded && decoded->noisePerMS == 0x1234 && decoded->agcCnt == 0x5678 && decoded->jamInd == 77);
    };
    hardware.operator()<ubx_payload_rx_mon_hw_ubx6_t>(53);
    hardware.operator()<ubx_payload_rx_mon_hw_ubx7_t>(45);
    CHECK(UBX::MessageCodec<ubx_payload_rx_nav_pvt_t>::decode(Bytes(84)));

    Bytes rf(28, 0);
    rf[1] = 1;
    CHECK(UBX::MessageCodec<ubx_payload_rx_mon_rf_t>::decode(rf));
    rf[1] = 2;
    CHECK(!UBX::MessageCodec<ubx_payload_rx_mon_rf_t>::decode(rf));
    rf[1] = 1;
    rf[0] = 99;
    CHECK(!UBX::MessageCodec<ubx_payload_rx_mon_rf_t>::decode(rf));
    Bytes comms(48, 0);
    comms[1] = 1;
    CHECK(UBX::MessageCodec<ubx_payload_rx_mon_comms_t>::decode(comms));
    comms[1] = 2;
    CHECK(!UBX::MessageCodec<ubx_payload_rx_mon_comms_t>::decode(comms));
    CHECK(UBX::MessageCodec<ubx_payload_rx_sec_sig_t>::decode(Bytes{2, 7, 0, 0}));
    CHECK(!UBX::MessageCodec<ubx_payload_rx_sec_sig_t>::decode(Bytes{99, 7, 0, 0}));
    CHECK(!UBX::MessageCodec<ubx_payload_rx_sec_sig_t>::decode(Bytes{2, 7, 0, 1}));
    CHECK(!UBX::MessageCodec<ubx_payload_rx_nav_sat_part2_t>::block(Bytes(12), 1));
    const auto ack = UBX::MessageCodec<ubx_payload_rx_ack_ack_t>::decode(Bytes{6, 0x24});
    CHECK(ack && ack->msg == UBX_MSG_CFG_NAV5);

    for (const auto [key, value] : std::array<UBX::ConfigurationValue, 5>{
             {{0x50000001, 0}, {0x10000001, 2}, {0x20000001, 256}, {0x30000001, 65536}, {0x00000001, 0}}}) {
        UBX::CheckedValsetBatch<32> batch;
        CHECK(batch.append(0x20000001, 1));
        const auto previousSize = batch.size;
        CHECK(!batch.append(key, value));
        CHECK(!batch.append(0x20000002, 2));
        CHECK(batch.size == previousSize && batch.payload().empty());
    }
    UBX::CheckedValsetBatch<28> batch;
    CHECK(batch.payload().empty());
    CHECK(batch.append(0x10000001, 1));
    CHECK(batch.append(0x20000002, 255));
    CHECK(batch.append(0x30000003, 65535));
    CHECK(batch.append(0x40000004, UINT32_MAX));
    CHECK(batch.size == batch.bytes.size());
    auto values = batch.bytes;
    CHECK(!UBX::decodeConfigurationValues(values));  // VALSET headers cannot masquerade as VALGET.
    values[0] = 1;
    values[1] = 0;
    const auto decoded = UBX::decodeConfigurationValues(values);
    CHECK(decoded && decoded->count == 4);
    CHECK(decoded->values[0].value == 1 && decoded->values[1].value == 255 && decoded->values[2].value == 65535 &&
          decoded->values[3].value == UINT32_MAX);
    for (size_t length = 1; length < values.size(); ++length) {
        if (length != 4 && length != 9 && length != 14 && length != 20) {
            CHECK(!UBX::decodeConfigurationValues(std::span(values).first(length)));
        }
    }
    values[8] = 2;
    CHECK(!UBX::decodeConfigurationValues(values));
    CHECK(!batch.append(0x20000005, 0));
    CHECK(batch.payload().empty());
    batch = {};
    CHECK(batch.append(0x20000005, 0));
    CHECK(!batch.payload().empty());
}

static void optionalCommandWriteEvidence()
{
    for (const auto key : {UBX_CFG_KEY_CFG_UART1INPROT_SPARTN, UBX_CFG_KEY_ODO_USE_ODO}) {
        for (const auto failure : {GPSWriteStatus::Error, GPSWriteStatus::Cancelled}) {
            ProtocolReceiver receiver;
            receiver.failValsetKey = key;
            receiver.valsetWriteFailure = failure;
            std::vector<GPSCommandResult> completions;
            auto io = receiver.io();
            io.commandFinished = [&](const GPSCommandResult& result) { completions.push_back(result); };
            GPSNativeUBX driver(std::move(io), false);
            GPSProtocol::GPSConfig config;
            std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
            std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).durationSecs = 60;
            unsigned baud = 115200;
            CHECK(!driver.configure(baud, config));
            CHECK(!completions.empty());
            const auto& result = completions.back();
            CHECK(result.evidence.command == std::to_string(UBX_MSG_CFG_VALSET));
            CHECK(!result.evidence.required);
            CHECK(result.evidence.outcome == (failure == GPSWriteStatus::Cancelled
                                                  ? GPSCommandOutcome::Cancelled
                                                  : GPSCommandOutcome::TransportError));
            CHECK(result.evidence.acceptedBytes == 9 && result.evidence.writtenBytes == 7);
            CHECK(result.evidence.uncertainBytes == 2);
            const auto count = completions.size();
            driver.finishConfigurationEvidence();
            CHECK(completions.size() == count);
        }
    }
}

const auto& testCases()
{
    static const struct
    {
        const char* name;
        void (*run)();
    } cases[] = {
        {"isolated-frame-control", isolatedFrameAndControl},
        {"checked-wire-codecs", checkedWireCodecs},
        {"optional-command-write-evidence", optionalCommandWriteEvidence},
        {"native-configuration-validation", invalidConfiguration},
        {"integrity-original-receipts", integrityReceipts},
        {"control-deadline", controlDeadline},
        {"identification-write-budget", identificationWriteBudget},
        {"explicit-no-fix", explicitNoFix},
        {"out-of-range-coordinates", outOfRangeCoordinates},
        {"read-only-baud-discovery", baudDiscovery},
        {"discovery-failures-and-late-acks", discoveryFailures},
        {"navigation-fix-flags-and-ordering", navigationFixFlags},
        {"transactional-frames", transactionalFrames},
        {"reentrant-payload", reentrantPayload},
        {"receive-read-diagnostics", receiveFailureLogging},
        {"comms-diagnostic-values", commsDiagnostics},
        {"comms-malformed-replies", invalidCommsDiagnostics},
        {"comms-expired-reply", expiredCommsDiagnostics},
        {"buffer-warning-rate-limit",
         [] {
             Fixture f;
             f.receiver.queueBufferWarning(false);
             f.driver.receive(100);
             CHECK(f.receiver.commsPolls == 0);
             f.receiver.queueBufferWarning();
             f.driver.receive(100);
             CHECK(f.receiver.commsPolls == 1);
             f.receiver.queueBufferWarning();
             f.driver.receive(100);
             CHECK(f.receiver.commsPolls == 1);
             gps_test_time += 5000000;
             f.receiver.queueBufferWarning();
             f.driver.receive(100);
             CHECK(f.receiver.commsPolls == 2);
         }},
        {"buffer-poll-failure-rate-limit",
         [] {
             Fixture f;
             f.receiver.failCommsWrite = true;
             f.receiver.queueBufferWarning();
             f.driver.receive(100);
             f.receiver.queueBufferWarning();
             f.driver.receive(100);
             CHECK(f.receiver.commsPolls == 1);
             gps_test_time += 5000000;
             f.receiver.failCommsWrite = false;
             f.receiver.queueBufferWarning();
             f.driver.receive(100);
             CHECK(f.receiver.commsPolls == 2);
         }},
        {"already-stopped",
         [] {
             Fixture f;
             f.success(1);
         }},
        {"poll-read-failure",
         [] {
             Fixture f;
             f.readFailure(GPSProtocolError::Transport);
         }},
        {"poll-read-cancelled",
         [] {
             Fixture f;
             f.readFailure(GPSProtocolError::Cancelled);
         }},
        {"silent-then-stopped",
         [] {
             Fixture f;
             f.receiver.surveyReplies = {SurveyReply::Silent, SurveyReply::Stopped};
             f.success(2);
         }},
        {"delayed-stop",
         [] {
             Fixture f;
             f.receiver.surveyReplies = {SurveyReply::Active, SurveyReply::Active, SurveyReply::Stopped};
             f.success(3);
             CHECK(f.receiver.startedAt - f.receiver.disabledAt >= 200000);
         }},
        {"completed-survey-is-not-stopped",
         [] {
             Fixture f;
             f.receiver.surveyReplies = {SurveyReply::Valid, SurveyReply::Stopped};
             f.success(2);
         }},
        {"bad-checksum-is-not-confirmation",
         [] {
             Fixture f;
             f.receiver.surveyReplies = {SurveyReply::BadChecksum, SurveyReply::Stopped};
             f.success(2);
         }},
        {"bad-length-is-not-confirmation",
         [] {
             Fixture f;
             f.receiver.surveyReplies = {SurveyReply::BadLength, SurveyReply::Stopped};
             f.success(2);
         }},
        {"active-timeout",
         [] {
             Fixture f;
             f.receiver.surveyReplies = {SurveyReply::Active};
             f.timeout();
         }},
        {"valid-timeout",
         [] {
             Fixture f;
             f.receiver.surveyReplies = {SurveyReply::Valid};
             f.timeout();
         }},
        {"silent-timeout",
         [] {
             Fixture f;
             f.receiver.surveyReplies = {SurveyReply::Silent};
             f.timeout();
         }},
        {"reconfigure-does-not-reuse-stop-confirmation",
         [] {
             Fixture f;
             f.success(1);
             f.receiver.resetState();
             f.receiver.surveyReplies = {SurveyReply::Silent};
             f.timeout();
         }},
        {"disable-nak",
         [] {
             Fixture f;
             f.receiver.rejectDisable = true;
             CHECK(!f.configure());
             CHECK(!f.driver.receiverReady());
             CHECK(f.receiver.modes == std::vector<uint32_t>({0}));
             CHECK(f.receiver.surveyPolls == 0 && f.receiver.starts == 0);
         }},
        {"start-nak",
         [] {
             Fixture f;
             f.receiver.rejectStart = true;
             CHECK(!f.configure());
             CHECK(!f.driver.receiverReady());
             CHECK(f.receiver.modes == std::vector<uint32_t>({0, 1}));
             CHECK(f.receiver.surveyPolls == 1 && f.receiver.starts == 1);
         }},
        {"poll-write-failure",
         [] {
             Fixture f;
             f.receiver.failPollWrite = true;
             CHECK(!f.configure());
             CHECK(!f.driver.receiverReady());
             CHECK(f.receiver.modes == std::vector<uint32_t>({0}));
             CHECK(f.receiver.starts == 0);
         }},
        {"configured-status-callback",
         [] {
             Fixture f;
             f.success(1);
             f.receiver.queueSurveyReply(SurveyReply::Active);
             f.driver.receive(100);
             CHECK(f.receiver.statusCallbacks == 1);
         }},
        {"configured-survey-activates-rtcm",
         [] {
             Fixture f;
             f.success(1);
             f.receiver.queueSurveyReply(SurveyReply::Valid);
             f.driver.receive(100);
             CHECK(f.receiver.statusCallbacks == 1);
             CHECK(f.receiver.rtcmEnables == 1);
             CHECK(!f.driver.hasIOError());
         }},
        {"fixed-base-does-not-poll",
         [] {
             Fixture f;
             f.base = {.mode = GPSBaseStationConfig::Fixed{
                           .position = {.latitudeDegrees = 47.0, .longitudeDegrees = 8.0, .altitudeMeters = 500.0f},
                           .accuracyMeters = 1.0f}};
             CHECK(f.configure());
             CHECK(f.driver.receiverReady());
             CHECK(f.receiver.modes == std::vector<uint32_t>({2}));
             CHECK(f.receiver.surveyPolls == 0 && f.receiver.starts == 0);
             CHECK(f.receiver.rtcmEnables == 1);
         }},
    };

    return cases;
}
}  // namespace

class GPSProtocolUbxTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _cases_data();
    void _cases();
};

void GPSProtocolUbxTest::_cases_data()
{
    QTest::addColumn<int>("index");
    const auto& cases = testCases();
    for (size_t index = 0; index < std::size(cases); ++index) {
        QTest::newRow(cases[index].name) << static_cast<int>(index);
    }
}

void GPSProtocolUbxTest::_cases()
{
    QFETCH(int, index);
    gps_test_time = 0;
    gps_test_warnings.clear();
    try {
        testCases()[index].run();
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolUbxTest, TestLabel::Unit)

#include "gps-ubx-test.moc"
