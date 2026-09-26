#pragma once

#include <array>
#include <chrono>
#include <span>
#include <string_view>

#include <QtCore/QByteArray>

#include "GPSCommandSequence.h"

struct GPSEllipsoidPosition;

/// Ashtech/Trimble receiver configuration as data, in the order it is sent. $PASHS commands set and $PASHQ commands
/// query; the receiver answers with $PASHR. "{port}" stands for the port letter the receiver reported in $PASHR,PRT.
///
/// Useful but unused: $PASHQ,VER (firmware), $PASHQ,OPTION (installed options; an MB-Two reports, among others,
/// W,20HZ and K,RTKBASE) and $PASHS,RST (factory reset, followed by a reboot of about 15 s).
namespace Ashtech::Plan {

struct Setting
{
    std::string_view command;
    bool required = true;
};

/// Per attempt, for every command.
inline constexpr std::chrono::milliseconds RESPONSE_TIMEOUT{200};

/// Rates the port query probes, in order. Trimble receivers default to 115200.
inline constexpr unsigned BAUD_RATES[] = {9600, 38400, 19200, 57600, 115200};
inline constexpr std::string_view PORT_QUERY = "$PASHQ,PRT";
inline constexpr unsigned PORT_QUERY_ATTEMPTS = 2;

/// The link ends at LINK_BAUD. From another rate, LINK_SPEED (SPD code 9) switches the receiver without a reply; the
/// host keeps receiving for LINK_SPEED_SETTLE, follows, and the port query confirms the receiver at the new rate.
inline constexpr unsigned LINK_BAUD = 115200;
inline constexpr std::string_view LINK_SPEED = "$PASHS,SPD,{port},9";
inline constexpr std::chrono::milliseconds LINK_SPEED_SETTLE{200};
inline constexpr unsigned PORT_QUERY_ATTEMPTS_AT_LINK_BAUD = 10;

/// Board identification; only an MB-Two ("MB2") is set up as a base station.
inline constexpr std::string_view BOARD_QUERY = "$PASHQ,RID";

/// Navigation output. Some receivers do not acknowledge some of these (GSV, for example), so none is required.
inline constexpr std::array OUTPUTS{
    Setting{"$PASHS,POP,20", false},                  // internal update rate 20 Hz
    Setting{"$PASHS,SNS,SOL", false},
    Setting{"$PASHS,NME,ALL,{port},OFF", false},      // all NMEA and NMEA-like messages off
    Setting{"$PASHS,ATM,ALL,{port},OFF", false},      // all ATOM messages off
    Setting{"$PASHS,OUT,{port},ON", false},           // periodic output on
    Setting{"$PASHS,NME,ZDA,{port},ON,3", false},     // date and time every 3 s
    Setting{"$PASHS,NME,GST,{port},ON,3", false},     // position accuracy every 3 s
    Setting{"$PASHS,NME,POS,{port},ON,0.05", false},  // position and velocity at 20 Hz (option W; 50 Hz needs 8)
    Setting{"$PASHS,NME,GSV,{port},ON,1", false},     // satellites in view every second
};

/// Survey-in base, sent after the first position: position averaging over the survey duration. Its $PASHR,RECEIPT
/// start receipt acknowledges it, and the finish receipt carries the averaged position.
inline constexpr std::string_view SURVEY_START = "$PASHS,POS,AVG,{seconds}";
/// Station description once averaging has started.
inline constexpr std::array STATION{
    Setting{"$PASHS,ANP,OWN,TRM55971.00"},  // antenna name (arbitrary)
    Setting{"$PASHS,STI,0001"},             // base station ID
};

/// Fixed base, sent after the first position (see fixedPosition()), then RTCM_OUTPUTS.
inline constexpr std::string_view FIXED_POSITION = "$PASHS,POS,{latitude},{N|S},{longitude},{E|W},{height},PC1";

/// RTCM3 output once the base position is known.
inline constexpr std::array RTCM_OUTPUTS{
    Setting{"$PASHS,NME,POS,{port},ON,0.2"},  // position output reduced to 5 Hz
    Setting{"$PASHS,RT3,1074,{port},ON,1"},   // GPS observations
    Setting{"$PASHS,RT3,1084,{port},ON,1"},   // GLONASS observations
    Setting{"$PASHS,RT3,1094,{port},ON,1"},   // Galileo observations
    Setting{"$PASHS,RT3,1114,{port},ON,1"},   // QZSS observations
    Setting{"$PASHS,RT3,1124,{port},ON,1"},   // BeiDou observations
    Setting{"$PASHS,RT3,1006,{port},ON,1"},   // static position
    Setting{"$PASHS,RT3,1033,{port},ON,31"},  // antenna and receiver name
    Setting{"$PASHS,RT3,1013,{port},ON,1"},   // system parameters
    Setting{"$PASHS,RT3,1029,{port},ON,1"},   // ASCII message
    Setting{"$PASHS,RT3,1230,{port},ON"},     // GLONASS code-phase biases
    // The u-blox base messages; possibly redundant for these receivers.
    Setting{"$PASHS,RT3,1005,{port},ON,1"},
    Setting{"$PASHS,RT3,1077,{port},ON,1"},
    Setting{"$PASHS,RT3,1087,{port},ON,1"},
};

/// A NAK rejects any command.
[[nodiscard]] GPSCommandOutcome rejection(std::string_view reply);

/// A $PASHS setting is acknowledged by an ACK.
[[nodiscard]] GPSCommandOutcome acknowledgement(std::string_view reply);

/// @a command as one CR/LF-terminated line.
[[nodiscard]] QByteArray line(std::string_view command);

/// @a command for @a port as one CR/LF-terminated line.
[[nodiscard]] QByteArray line(std::string_view command, char port);

/// The SURVEY_START line for @a duration.
[[nodiscard]] QByteArray surveyStart(std::chrono::seconds duration);

/// The FIXED_POSITION line for @a position: unsigned ddmm.mmmmmmmm and dddmm.mmmmmmmm with hemisphere letters, and
/// the height in metres.
[[nodiscard]] QByteArray fixedPosition(const GPSEllipsoidPosition& position);

/// One command @a line, labelled by its text and answered through @a reply.
[[nodiscard]] GPSCommandSequence::Command command(const QByteArray& line, GPSTextMatcher reply = acknowledgement,
                                                  bool required = true);

/// The fixedPosition() command, labelled without its coordinates.
[[nodiscard]] GPSCommandSequence::Command fixedPositionCommand(const GPSEllipsoidPosition& position);

/// @a settings for @a port, each acknowledged by an ACK.
[[nodiscard]] GPSCommandSequence sequence(std::span<const Setting> settings, char port);

}  // namespace Ashtech::Plan
