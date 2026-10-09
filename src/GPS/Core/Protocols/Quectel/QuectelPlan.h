#pragma once

#include <array>
#include <chrono>
#include <span>
#include <string_view>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>

#include "GPSCommand.h"
#include "GPSProtocolMath.h"
#include "GPSReceiverConfig.h"

/// LG290P(03) base-station configuration, from Quectel's LG290P(03)&LGx80P(03) GNSS Protocol Specification V1.1
/// (§§2.3.9, 2.3.15, 2.3.22–25, 2.3.28) and Base Station Mode Application Note V1.1 §3.2. Commands are PQTM sentence
/// bodies, sent as "$<body>*hh\r\n"; the receiver answers "<name>,OK[,<values>]" or "<name>,ERROR,<code>".
///
/// Role and base settings take effect only once saved with SAVE and restarted with RESTART, so configuration reads
/// them back after a restart before trusting them. It changes and saves them only with consent to persistent changes;
/// with consent it first restarts, so it works from the saved settings rather than another client's unsaved ones.
/// The note permits re-running an unchanged survey (RESTART_SURVEY) and restarting without saving.
namespace Quectel::Plan {

/// Per attempt, unless a command gives its own.
inline constexpr std::chrono::milliseconds COMMAND_TIMEOUT{1000};
/// For the whole configuration after validation.
inline constexpr std::chrono::milliseconds CONFIGURATION_TIMEOUT{45000};
/// Rates the identity query probes, in order.
inline constexpr std::array BAUD_RATES{460800u, 115200u, 230400u, 921600u, 57600u, 9600u};

/// Firmware identity: "PQTMVERNO,<firmware>,<date>,<time>". Only LG290P(03) firmware is qualified; LG580P and LG680P
/// use the same commands.
inline constexpr std::string_view IDENTIFY = "PQTMVERNO";
inline constexpr std::string_view QUALIFIED_FIRMWARE = "LG290P03";

/// Receiver role: "PQTMCFGRCVRMODE,OK,<role>", where 2 is the base station.
inline constexpr std::string_view READ_ROLE = "PQTMCFGRCVRMODE,R";
inline constexpr unsigned BASE_ROLE = 2;
inline constexpr std::string_view WRITE_BASE_ROLE = "PQTMCFGRCVRMODE,W,2";

/// Base positioning runs at 1 Hz: "PQTMCFGFIXRATE,OK,1000".
inline constexpr std::string_view READ_FIX_RATE = "PQTMCFGFIXRATE,R";
inline constexpr unsigned FIX_INTERVAL_MS = 1000;

/// Base mode: "PQTMCFGSVIN,OK,<mode>,<observations>,<accuracy>,<x>,<y>,<z>[,<distance>]", with mode 1 survey-in and 2
/// fixed ECEF coordinates. Survey-in counts accepted 1 Hz fixes; its accuracy threshold filters each 3D fix.
inline constexpr std::string_view READ_BASE = "PQTMCFGSVIN,R";
inline constexpr std::chrono::hours MAXIMUM_SURVEY_DURATION{24};
inline constexpr double MAXIMUM_SURVEY_ACCURACY_METERS = 1000;
/// Re-runs a survey with exactly the fields READ_BASE reported.
inline constexpr std::string_view RESTART_SURVEY = "PQTMCFGSVIN,W";
/// Labels for the base writes whose bodies carry ECEF coordinates.
inline constexpr std::string_view FIXED_BASE_LABEL = "PQTMCFGSVIN,W (fixed position)";
inline constexpr std::string_view RESTART_SURVEY_LABEL = "PQTMCFGSVIN,W (survey restart)";

/// Saves the current settings to flash.
inline constexpr std::string_view SAVE = "PQTMSAVEPAR";
inline constexpr std::chrono::milliseconds SAVE_TIMEOUT{5000};

/// Restarts without an acknowledgement. The identity is queried every RESTART_POLL, for up to
/// RESTART_IDENTIFY_TIMEOUT, until it answers after the boot banner "PQTMVER,1,MODULE,<firmware>,...". Then the
/// role is read back. All of it within RESTART_TIMEOUT.
inline constexpr std::string_view RESTART = "PQTMSRR";
inline constexpr std::chrono::milliseconds RESTART_TIMEOUT{8000};
inline constexpr std::chrono::milliseconds RESTART_POLL{200};
inline constexpr std::chrono::milliseconds RESTART_IDENTIFY_TIMEOUT{700};

struct MessageRate
{
    std::string_view message;
    unsigned rate = 1;
    /// Message version; standard NMEA messages have none.
    std::string_view version{};
};

/// Base output: survey-in status, RTCM3 station coordinates and MSM observations.
inline constexpr std::array BASE_OUTPUT{
    MessageRate{.message = "PQTMSVINSTATUS", .version = "1"},
    MessageRate{.message = "RTCM3-1005"},
    MessageRate{.message = "RTCM3-107X", .version = "0"},
};

/// NMEA output: position, position accuracy, satellites used and satellites in view, every second.
inline constexpr std::array NMEA_OUTPUT{
    MessageRate{.message = "GGA"},
    MessageRate{.message = "GST"},
    MessageRate{.message = "GSA"},
    MessageRate{.message = "GSV"},
};

/// The PQTMCFGSVIN,W body for @a mode: fixed @a position ECEF coordinates, or a survey-in by observation count and
/// accuracy. @a distanceField adds a zero distance field, for firmware whose READ_BASE reply has one.
[[nodiscard]] QByteArray writeBase(const GPSBaseStationConfig::Mode& mode, const GPSProtocolMath::Ecef& position,
                                   bool distanceField);

/// @a body labelled by its text, framed as a sentence, and completed by @a reply within @a timeout.
[[nodiscard]] GPSCommandSequence::Command command(QByteArrayView body, GPSTextMatcher reply,
                                                  std::chrono::milliseconds timeout = COMMAND_TIMEOUT);

/// Completes the setting @a body with its "<name>,OK" acknowledgement.
[[nodiscard]] GPSTextMatcher acknowledgement(QByteArrayView body);

/// PQTMCFGMSGRATE,W for each of @a rates, each followed by its PQTMCFGMSGRATE,R readback.
[[nodiscard]] GPSCommandSequence messageRates(std::span<const MessageRate> rates);

}  // namespace Quectel::Plan
