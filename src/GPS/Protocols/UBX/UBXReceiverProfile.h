#pragma once

#include <array>
#include <cstdint>
#include <string_view>

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
    /// A USB port, and raw measurement output; M10 and F10 have neither.
    bool usb = true;
    /// RTCM3 base output (F9P and X20). Other receivers reject the RTCM3X output protocol keys, and M9 SPG rejects the
    /// whole CFG-VALSET that contains them, even with a value of zero.
    bool rtcmOutput = false;
    /// The board alone tells whether it can be a base station.
    bool baseCapabilityKnown = false;
    /// NAV-HPPOSLLH and NAV-RELPOSNED; not on M9 SPG, M10 or F10.
    bool highPrecision = true;
    /// CFG-ODO beyond USE_ODO and PROFILE; M9 SPG has only those two.
    bool fullOdometer = true;
    /// u-blox 9 reports corrections in RXM-RTCM where it rejects RXM-COR.
    bool rxmRtcmFallback = false;
    /// GPS L5 is broadcast unhealthy while pre-operational; L1/L5 receivers take the L1 health flag instead.
    bool l5HealthOverride = false;
    /// Dual-antenna heading (NAV-DAHEADING) and Galileo HAS host-correction keys (X20).
    bool dualAntennaHeading = false;
    /// Rover measurement period (CFG-RATE-MEAS). M9N computes 8 Hz with all satellites; F9P computes 5 Hz with RTK and
    /// four constellations on firmware 1.50 and later; X20 could run at 25 Hz, but drops out at 115200 baud with RTK.
    uint16_t measurementPeriodMs = 100;
    /// Configures through CFG-VALSET unless MON-VER reports an older protocol version.
    bool protocol27 = false;
    /// Has no time mode unless MON-VER reports otherwise.
    bool timeModeUnsupported = false;
};

inline constexpr std::array RECEIVER_PROFILES = {
    ReceiverProfile{},
    ReceiverProfile{.board = Board::u_blox5, .hardwareVersion = "00040005", .timeModeUnsupported = true},
    ReceiverProfile{.board = Board::u_blox6, .hardwareVersion = "00040007", .timeModeUnsupported = true},
    ReceiverProfile{.board = Board::u_blox7, .hardwareVersion = "00070000", .timeModeUnsupported = true},
    ReceiverProfile{.board = Board::u_blox8, .hardwareVersion = "00080000"},
    ReceiverProfile{.board = Board::u_blox9,
                    .hardwareVersion = "00190000",
                    .baseCapabilityKnown = true,
                    .highPrecision = false,
                    .fullOdometer = false,
                    .rxmRtcmFallback = true,
                    .measurementPeriodMs = 125,
                    .protocol27 = true},
    ReceiverProfile{.board = Board::u_blox9_F9P_L1L2,
                    .rtcmOutput = true,
                    .baseCapabilityKnown = true,
                    .rxmRtcmFallback = true,
                    .measurementPeriodMs = 200,
                    .protocol27 = true},
    ReceiverProfile{.board = Board::u_blox10,
                    .hardwareVersion = "000A0000",
                    .usb = false,
                    .baseCapabilityKnown = true,
                    .highPrecision = false,
                    .protocol27 = true,
                    .timeModeUnsupported = true},
    ReceiverProfile{.board = Board::u_blox9_F9P_L1L5,
                    .rtcmOutput = true,
                    .baseCapabilityKnown = true,
                    .rxmRtcmFallback = true,
                    .l5HealthOverride = true,
                    .measurementPeriodMs = 200,
                    .protocol27 = true},
    ReceiverProfile{.board = Board::u_blox10_L1L5,
                    .usb = false,
                    .baseCapabilityKnown = true,
                    .highPrecision = false,
                    .l5HealthOverride = true,
                    .protocol27 = true,
                    .timeModeUnsupported = true},
    ReceiverProfile{.board = Board::u_blox_X20,
                    .hardwareVersion = "000B0000",
                    .rtcmOutput = true,
                    .baseCapabilityKnown = true,
                    .l5HealthOverride = true,
                    .dualAntennaHeading = true,
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
