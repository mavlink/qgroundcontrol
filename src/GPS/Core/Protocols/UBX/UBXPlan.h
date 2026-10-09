#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <vector>

#include "GPSReceiverConfig.h"
#include "UBX/UBXConfigKeys.h"
#include "UBX/UBXFrame.h"
#include "UBX/UBXMessageSchema.h"

namespace UBX {
enum class Board : uint8_t
{
    unknown = 0,
    u_blox5 = 5,
    u_blox6 = 6,
    u_blox7 = 7,
    u_blox8 = 8,
    u_blox9 = 9,
    u_blox9_F9P_L1L2 = 10,
    u_blox10 = 11,
    u_blox9_F9P_L1L5 = 12,
    u_blox10_L1L5 = 13,
    u_blox_X20 = 14
};

/// What configuration may rely on for a board. The MON-VER hardware version selects a generation; the firmware and
/// module strings then refine u-blox 9 to F9P and u-blox 10 to DAN-F10N.
struct ReceiverProfile
{
    Board board = Board::unknown;
    /// MON-VER hwVersion of a generation; refined boards have none.
    std::string_view hardwareVersion{};
    /// RTCM3 base output (F9P and X20); only these receivers are configured as a base.
    bool rtcmOutput = false;
    /// The board alone tells whether it can be a base station.
    bool baseCapabilityKnown = false;
    /// GPS L5 is broadcast unhealthy while pre-operational; L1/L5 receivers take the L1 health flag instead.
    bool l5HealthOverride = false;
    /// Measurement period (CFG-RATE-MEAS) until RTCM output starts. F9P computes 5 Hz with RTK and four constellations
    /// on firmware 1.50 and later; X20 could run at 25 Hz, but drops out at 115200 baud with RTK.
    uint16_t measurementPeriodMs = 100;
    /// Configures through CFG-VALSET unless MON-VER reports an older protocol version.
    bool protocol27 = false;
};

inline constexpr std::array RECEIVER_PROFILES = {
    ReceiverProfile{},
    // Generations before u-blox 8 cannot output RTCM, so they are never configured as a base.
    ReceiverProfile{.board = Board::u_blox5, .hardwareVersion = "00040005", .baseCapabilityKnown = true},
    ReceiverProfile{.board = Board::u_blox6, .hardwareVersion = "00040007", .baseCapabilityKnown = true},
    ReceiverProfile{.board = Board::u_blox7, .hardwareVersion = "00070000", .baseCapabilityKnown = true},
    ReceiverProfile{.board = Board::u_blox8, .hardwareVersion = "00080000"},
    ReceiverProfile{
        .board = Board::u_blox9, .hardwareVersion = "00190000", .baseCapabilityKnown = true, .protocol27 = true},
    ReceiverProfile{.board = Board::u_blox9_F9P_L1L2,
                    .rtcmOutput = true,
                    .baseCapabilityKnown = true,
                    .measurementPeriodMs = 200,
                    .protocol27 = true},
    ReceiverProfile{
        .board = Board::u_blox10, .hardwareVersion = "000A0000", .baseCapabilityKnown = true, .protocol27 = true},
    ReceiverProfile{.board = Board::u_blox9_F9P_L1L5,
                    .rtcmOutput = true,
                    .baseCapabilityKnown = true,
                    .l5HealthOverride = true,
                    .measurementPeriodMs = 200,
                    .protocol27 = true},
    ReceiverProfile{.board = Board::u_blox10_L1L5, .baseCapabilityKnown = true, .protocol27 = true},
    ReceiverProfile{.board = Board::u_blox_X20,
                    .hardwareVersion = "000B0000",
                    .rtcmOutput = true,
                    .baseCapabilityKnown = true,
                    .l5HealthOverride = true,
                    .protocol27 = true},
};

[[nodiscard]] constexpr ReceiverProfile receiverProfile(Board board)
{
    for (const auto& profile : RECEIVER_PROFILES) {
        if (profile.board == board) {
            return profile;
        }
    }
    return RECEIVER_PROFILES.front();
}

/// The generation whose MON-VER hwVersion is @a hardwareVersion; unknown when none matches.
[[nodiscard]] constexpr Board boardFromHardwareVersion(std::string_view hardwareVersion)
{
    for (const auto& profile : RECEIVER_PROFILES) {
        if (!profile.hardwareVersion.empty() && profile.hardwareVersion == hardwareVersion) {
            return profile.board;
        }
    }
    return Board::unknown;
}
}  // namespace UBX

/// u-blox receiver configuration as data: the CFG-VALSET batches of protocol 27 and later, the CFG-PRT, CFG-RATE,
/// CFG-NAV5, CFG-MSG and CFG-TMODE3 commands of older receivers, and the timeouts and values they use. Every VALSET
/// batch is one message on the wire, so its boundaries are part of the configuration.
namespace UBX::Plan {

/// Acknowledgement timeout of one configuration command.
inline constexpr std::chrono::milliseconds CONFIG_TIMEOUT{250};
/// The u-blox 8 protocol has CFG commands acknowledged within one second.
inline constexpr std::chrono::milliseconds LEGACY_REPLY_TIMEOUT{1000};
/// Slow factory NMEA output can delay a MON-VER response by over a second.
inline constexpr std::chrono::milliseconds IDENTITY_TIMEOUT{2000};
inline constexpr std::chrono::milliseconds PORT_TIMEOUT{2000};
/// Traffic the baud probe drains before polling MON-VER.
inline constexpr std::chrono::milliseconds BAUD_PROBE_DRAIN{20};
inline constexpr std::chrono::milliseconds SURVEY_STOP_TIMEOUT{3000};
inline constexpr std::chrono::milliseconds SURVEY_STOP_POLL{100};
/// An unwanted message is disabled at most this often; some cannot be disabled.
inline constexpr std::chrono::microseconds DISABLE_MESSAGE_INTERVAL = std::chrono::seconds(1);

/// Probing order of the automatic baud detection.
inline constexpr std::array BAUD_RATES{38400u, 57600u, 9600u, 115200u, 230400u, 460800u, 921600u};
/// The link rate a base is moved to when no rate is selected.
inline constexpr unsigned BASE_BAUD = 115200;

/// Bounds a CFG-VALSET independently of received message sizes.
inline constexpr size_t VALSET_CAPACITY = 256;

inline constexpr uint8_t NO_OUTPUT = 0;
/// Base stations always use the stationary navigation model.
inline constexpr uint8_t STATIONARY_DYNAMIC_MODEL = 2;
/// NAV-SAT every tenth survey epoch.
inline constexpr uint8_t SURVEY_SATELLITE_RATE = 10;
/// Satellites every 2 s at the 1 Hz base rate, as during a 5 Hz survey; the survey divisor would exceed the 5 s
/// freshness of satellite reports.
inline constexpr uint8_t BASE_SATELLITE_INFO_RATE = 2;
inline constexpr uint8_t SURVEY_STATUS_RATE = 5;
/// RTCM output only needs 1 Hz; the survey keeps the faster navigation rate because it converges faster.
inline constexpr uint16_t BASE_MEASUREMENT_PERIOD_MS = 1000;

/// One CFG-VALSET key and value. The value has the key's type and braced items reject narrowing, so a value that does
/// not fit the key's width does not compile. A CFG-MSGOUT rate expands to UART1 and USB.
struct ValsetItem
{
    template <typename T>
    constexpr ValsetItem(CfgKey<T> setting, std::type_identity_t<T> setValue, bool when = true)
        : key(setting.id)
        , value(static_cast<uint32_t>(static_cast<std::make_unsigned_t<T>>(setValue)))
        , included(when)
    {}

    constexpr ValsetItem(MsgOutKey output, uint8_t rate, bool when = true)
        : key(output.i2c.id)
        , value(rate)
        , msgOut(true)
        , included(when)
    {}

    /// The key, or the I2C key of a CFG-MSGOUT family.
    uint32_t key;
    uint32_t value;
    bool msgOut = false;
    bool included = true;
};

/// What a batch that is rejected, unanswered or not written means for configuration.
enum class NakPolicy : uint8_t
{
    Fail,       ///< Required: configuration stops.
    Tolerate,   ///< Optional: firmware without these keys rejects them, or the configuration flow decides.
    Warn,       ///< Optional, but worth a warning.
    WriteOnly,  ///< Required, but written without awaiting the reply.
};

struct ValsetBatch
{
    /// Whether the receiver takes this batch at all.
    bool included = true;
    NakPolicy policy = NakPolicy::Fail;
    std::chrono::milliseconds timeout = CONFIG_TIMEOUT;
    const char* warning = nullptr;
    std::vector<ValsetItem> items{};

    [[nodiscard]] bool required() const { return policy == NakPolicy::Fail || policy == NakPolicy::WriteOnly; }
};

/// Protocol 27+ port setup at the detected baud rate: UBX only on UART1, 8N1.
[[nodiscard]] ValsetBatch uart1Protocols();
[[nodiscard]] ValsetBatch uart1Baudrate(uint32_t baudrate);
/// Protocol 27+ configuration before jamming detection: ports, navigation, odometer and RTK.
[[nodiscard]] std::vector<ValsetBatch> portsAndNavigation(const ReceiverProfile& profile);
/// Probes CFG-SEC-JAMDET, which F9 HPG 1.50+, F9 L1L5 and X20 have instead of CFG-ITFM.
[[nodiscard]] ValsetBatch jammingDetection();
/// Enables the interference monitor of firmware without CFG-SEC-JAMDET.
[[nodiscard]] ValsetBatch interferenceMonitor();
/// Protocol 27+ configuration after jamming detection: message output and receiver-specific extras.
[[nodiscard]] std::vector<ValsetBatch> messageOutput(const ReceiverProfile& profile);
/// Stops RTCM output, including observations a previous session left in the other MSM format.
[[nodiscard]] ValsetBatch disableRTCMOutput();
[[nodiscard]] ValsetBatch disableTimeMode();
[[nodiscard]] ValsetBatch surveyIn(const GPSBaseStationConfig::SurveyIn& survey);
[[nodiscard]] ValsetBatch fixedBase(const GPSBaseStationConfig::Fixed& fixed);
/// Base output at 1 Hz: station position, observations in the selected MSM format and GLONASS biases.
[[nodiscard]] ValsetBatch rtcmOutput(bool compactObservations);

/// Messages a protocol 27+ receiver can send unasked and their CFG-MSGOUT keys, which a message ID does not imply.
struct UnexpectedOutput
{
    MessageId message;
    MsgOutKey key;
};

inline constexpr std::array<UnexpectedOutput, 3> UNEXPECTED_OUTPUT{{{Msg::RXM_RAWX, Cfg::MSGOUT_UBX_RXM_RAWX},
                                                                    {Msg::RXM_SFRBX, Cfg::MSGOUT_UBX_RXM_SFRBX},
                                                                    {Msg::NAV_TIMEGPS, Cfg::MSGOUT_UBX_NAV_TIMEGPS}}};

/// Disables unexpected output; the reply is not awaited.
[[nodiscard]] ValsetBatch disableMessage(MsgOutKey key);

/// CFG-MSG output rate of one message, as a divisor of the measurement rate.
struct MessageRate
{
    MessageId message;
    uint8_t rate;
};

inline constexpr uint16_t LEGACY_PORT_UART1 = 1;
inline constexpr uint16_t LEGACY_PORT_USB = 3;

/// CFG-PRT of UART1 and USB as one message, which older receivers take: UBX in, UBX and RTCM3 out, and NMEA out on USB
/// as the protocol 27+ configuration keeps it.
[[nodiscard]] std::array<uint8_t, 2 * WIRE_SIZE<CfgPrt>> legacyPorts(uint32_t baudrate);

/// 5 Hz, UTC-aligned.
inline constexpr CfgRate LEGACY_SURVEY_RATE{.measRate = 200, .navRate = 1, .timeRef = 0};
inline constexpr CfgRate LEGACY_BASE_RATE{.measRate = BASE_MEASUREMENT_PERIOD_MS, .navRate = 1, .timeRef = 0};
/// Updates only the dynamic model and the fix mode (3D only).
inline constexpr CfgNav5 LEGACY_NAVIGATION{.mask = 0x0005, .dynModel = STATIONARY_DYNAMIC_MODEL, .fixMode = 2};
/// NAV-PVT, status, satellite and MON-HW (antenna and jamming) rates during the survey.
[[nodiscard]] std::array<MessageRate, 5> legacyMessageOutput();
inline constexpr MessageRate LEGACY_SURVEY_STATUS{Msg::NAV_SVIN, SURVEY_STATUS_RATE};
/// Written without waiting for acknowledgements.
[[nodiscard]] std::array<MessageRate, 10> legacyDisableRTCMOutput();
/// Survey status and satellite rates for the 1 Hz base; rejections are ignored.
[[nodiscard]] std::array<MessageRate, 2> legacyBaseStatus();

/// What a legacy RTCM output gives a rover. A base needs the station position and the observations of at least one
/// constellation; M8P firmware versions differ in the RTCM messages they support, so the others may be rejected.
enum class RTCMContent : uint8_t
{
    StationPosition,
    Observations,
    Optional,
};

struct RTCMOutput
{
    MessageRate rate;
    RTCMContent content;
};

[[nodiscard]] std::array<RTCMOutput, 6> legacyRTCMOutput(bool compactObservations);
[[nodiscard]] CfgTmode3 legacySurveyIn(const GPSBaseStationConfig::SurveyIn& survey);
[[nodiscard]] CfgTmode3 legacyFixedBase(const GPSBaseStationConfig::Fixed& fixed);

}  // namespace UBX::Plan
