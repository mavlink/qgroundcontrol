#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "GPSBaseStationConfig.h"
#include "UBX/Generated/UBXConfigKeys.h"
#include "UBX/Generated/UBXMessageIds.h"
#include "UBXMessageSchema.h"
#include "UBXReceiverProfile.h"

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
inline constexpr std::chrono::microseconds DISABLE_MESSAGE_INTERVAL{1000000};
/// A congested receiver must not be flooded with MON-COMMS polls.
inline constexpr std::chrono::microseconds COMMS_POLL_INTERVAL{5000000};
inline constexpr std::chrono::microseconds COMMS_REPLY_WINDOW{2000000};

/// Probing order of the automatic baud detection.
inline constexpr std::array<unsigned, 7> BAUD_RATES{38400, 57600, 9600, 115200, 230400, 460800, 921600};
/// The link rate a base is moved to when no rate is selected.
inline constexpr unsigned BASE_BAUD = 115200;

/// Bounds a CFG-VALSET independently of received message sizes.
inline constexpr size_t VALSET_CAPACITY = 256;

inline constexpr uint8_t NO_OUTPUT = 0;
/// Base stations always use the stationary navigation model.
inline constexpr uint8_t STATIONARY_DYNAMIC_MODEL = 2;
/// NAV-SAT every tenth rover epoch.
inline constexpr uint8_t ROVER_SATELLITE_RATE = 10;
/// Satellites every 2 s at the 1 Hz base rate, as during a 5 Hz survey; the rover divisor would exceed the 5 s
/// freshness of satellite reports.
inline constexpr uint8_t BASE_SATELLITE_INFO_RATE = 2;
inline constexpr uint8_t SURVEY_STATUS_RATE = 5;
/// RTCM output only needs 1 Hz; the survey keeps the rover rate because it converges faster.
inline constexpr uint16_t BASE_MEASUREMENT_PERIOD_MS = 1000;

/// Fixed-position accuracy in 0.1 mm.
[[nodiscard]] inline uint32_t fixedAccuracyWireUnits(float accuracyMeters)
{
    // Match shared validation: multiplying directly by 10000 can round an accepted value past UINT32_MAX.
    const float accuracyMillimeters = accuracyMeters * 1000.0f;
    return static_cast<uint32_t>(accuracyMillimeters * 10.0f);
}

/// Survey-in accuracy limit in 0.1 mm.
[[nodiscard]] inline uint32_t surveyAccuracyWireUnits(double accuracyMeters)
{
    return static_cast<uint32_t>(accuracyMeters * 10000.0);
}

/// Fixed position: latitude and longitude in 1e-7 deg and height in cm, each with a high-precision remainder
/// (1e-9 deg, 0.1 mm) in [-99, 99].
struct FixedPositionWire
{
    int32_t latitude;
    int8_t latitudeHp;
    int32_t longitude;
    int8_t longitudeHp;
    int32_t height;
    int8_t heightHp;
};

[[nodiscard]] FixedPositionWire fixedPositionWire(const GPSEllipsoidPosition& position);

/// The receiver facts that select plan items.
struct Target
{
    ReceiverProfile profile{};
    bool satelliteInfo = false;
};

/// One CFG-VALSET key and value. The value has the key's type and braced items reject narrowing, so a value that does
/// not fit the key's width does not compile. A CFG-MSGOUT rate expands to UART1 and, on boards with one, USB.
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
    Fail,         ///< Required: configuration stops.
    Tolerate,     ///< Optional: firmware without these keys rejects them.
    Warn,         ///< Optional, but worth a warning.
    IgnoreReply,  ///< Optional reply, but the write must succeed.
    Report,       ///< Optional; the configuration flow decides what a rejection means.
    WriteOnly,    ///< Required, but written without awaiting the reply.
};

struct ValsetBatch
{
    /// Whether the receiver takes this batch at all.
    bool included = true;
    NakPolicy policy = NakPolicy::Fail;
    /// Sent only when the batch before it was not acknowledged.
    bool fallback = false;
    std::chrono::milliseconds timeout = CONFIG_TIMEOUT;
    const char* warning = nullptr;
    std::vector<ValsetItem> items{};

    [[nodiscard]] bool required() const { return policy == NakPolicy::Fail || policy == NakPolicy::WriteOnly; }
};

/// Protocol 27+ port setup at the detected baud rate: UBX only on UART1, 8N1.
[[nodiscard]] ValsetBatch uart1Protocols();
[[nodiscard]] ValsetBatch uart1Baudrate(uint32_t baudrate);
/// Protocol 27+ rover configuration before jamming detection: ports, navigation, odometer and RTK.
[[nodiscard]] std::vector<ValsetBatch> portsAndNavigation(const Target& target);
/// Probes CFG-SEC-JAMDET, which F9 HPG 1.50+, F9 L1L5 and X20 have instead of CFG-ITFM.
[[nodiscard]] ValsetBatch jammingDetection();
/// Enables the interference monitor of firmware without CFG-SEC-JAMDET.
[[nodiscard]] ValsetBatch interferenceMonitor();
/// Protocol 27+ rover configuration after jamming detection: message output and receiver-specific extras.
[[nodiscard]] std::vector<ValsetBatch> messageOutput(const Target& target);
/// Stops RTCM output, including observations a previous session left in the other MSM format.
[[nodiscard]] ValsetBatch disableRTCMOutput();
[[nodiscard]] ValsetBatch disableTimeMode();
[[nodiscard]] ValsetBatch surveyIn(const GPSBaseStationConfig::SurveyIn& survey);
[[nodiscard]] ValsetBatch fixedBase(const GPSBaseStationConfig::Fixed& fixed);
/// Base output at 1 Hz: station position, observations in the selected MSM format and GLONASS biases.
[[nodiscard]] ValsetBatch rtcmOutput(const Target& target, bool compactObservations);

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
inline constexpr uint32_t LEGACY_PORT_MODE_8N1 = 0x08d0;
inline constexpr uint16_t LEGACY_PROTOCOL_UBX = 1 << 0;
inline constexpr uint16_t LEGACY_PROTOCOL_RTCM3 = 1 << 5;

/// CFG-PRT of UART1 and USB: UBX in, UBX and RTCM3 out.
[[nodiscard]] std::array<CfgPrt, 2> legacyPorts(uint32_t baudrate);

/// 5 Hz, UTC-aligned.
inline constexpr CfgRate LEGACY_ROVER_RATE{.measRate = 200, .navRate = 1, .timeRef = 0};
inline constexpr CfgRate LEGACY_BASE_RATE{.measRate = BASE_MEASUREMENT_PERIOD_MS, .navRate = 1, .timeRef = 0};
/// Updates only the dynamic model and the fix mode (3D only).
inline constexpr CfgNav5 LEGACY_NAVIGATION{.mask = 0x0005, .dynModel = STATIONARY_DYNAMIC_MODEL, .fixMode = 2};
/// NAV-PVT exists from u-blox 7; a rejection selects the separate position, solution, velocity and time messages.
inline constexpr MessageRate LEGACY_NAV_PVT{Msg::NAV_PVT, 1};
inline constexpr std::array<MessageRate, 4> LEGACY_POSITION_WITHOUT_PVT{
    {{Msg::NAV_TIMEUTC, 5}, {Msg::NAV_POSLLH, 1}, {Msg::NAV_SOL, 1}, {Msg::NAV_VELNED, 1}}};
[[nodiscard]] std::array<MessageRate, 4> legacyStatus(const Target& target);
inline constexpr MessageRate LEGACY_SURVEY_STATUS{Msg::NAV_SVIN, SURVEY_STATUS_RATE};
/// Written without waiting for acknowledgements.
[[nodiscard]] std::array<MessageRate, 10> legacyDisableRTCMOutput();
/// Survey status and satellite rates for the 1 Hz base; rejections are ignored.
[[nodiscard]] std::array<MessageRate, 2> legacyBaseStatus(const Target& target);

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
