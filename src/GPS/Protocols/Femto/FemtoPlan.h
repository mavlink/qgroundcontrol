#pragma once

#include <array>
#include <chrono>
#include <span>
#include <string_view>

#include <QtCore/QByteArray>

#include "GPSCommandSequence.h"

struct GPSEllipsoidPosition;

/// Femtomes receiver configuration as data, in the order it is sent. Replies are ASCII text that need not be NMEA
/// sentences, so each command is acknowledged by its reply text in the raw stream and rejected by REJECTED.
namespace Femto::Plan {

struct Command
{
    std::string_view line;
    std::string_view accepted;
};

/// Per attempt, for every command.
inline constexpr std::chrono::milliseconds RESPONSE_TIMEOUT{200};
inline constexpr std::string_view REJECTED = "<ERROR";

/// The only rate the receivers are configured at.
inline constexpr unsigned BAUD = 115200;
inline constexpr unsigned BAUD_RATES[] = {BAUD};

/// Stops every log on this port and identifies the receiver, in up to IDENTIFY_ROUNDS rounds.
inline constexpr std::array IDENTIFY{
    Command{"UNLOGALL THISPORT\r\n", "<UNLOGALL OK"},
    Command{"VERSION\r\n", "<VERSION OK"},
};
inline constexpr unsigned IDENTIFY_ROUNDS = 2;

/// GGA output; the base station uses it only to observe the survey-in.
inline constexpr Command GGA_OUTPUT{"LOG GPGGA 1 \r\n", "<LOG OK"};

/// Survey-in base: receiver position averaging. A GGA fix of quality 7 completes it, and RTCM_OUTPUT follows.
inline constexpr std::array SURVEY_IN{
    Command{"POSAVE ON \r\n", "<POSAVE OK"},
    GGA_OUTPUT,
};

/// Fixed base (see fixedBase()): the position in degrees and ellipsoid height in metres, GGA_OUTPUT, then
/// RTCM_OUTPUT.
inline constexpr std::string_view FIX_POSITION = "FIX POSITION {latitude} {longitude} {height}\r\n";
inline constexpr std::string_view FIX_POSITION_ACCEPTED = "FIX OK";

/// RTCM3 output once the base position is known.
inline constexpr std::array RTCM_OUTPUT{
    Command{"LOG RTCM 1\r\n", "<LOG OK"},
};

[[nodiscard]] GPSCommandSequence sequence(std::span<const Command> commands);

/// FIX_POSITION for @a position, then GGA_OUTPUT.
[[nodiscard]] GPSCommandSequence fixedBase(const GPSEllipsoidPosition& position);

}  // namespace Femto::Plan
