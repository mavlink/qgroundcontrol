#pragma once

#include <array>
#include <chrono>
#include <cstdint>

#include <QtCore/QByteArray>
#include <QtCore/QList>

#include "GPSProtocolMath.h"

/// UM980/UM982 base-station configuration as command lists, in the order they are sent, from the Unicore N4 Commands
/// and Logs Reference Book EN R1.6 (sections 3, 7.3.1, 7.3.27 and 7.3.44). Each command is one "<text>\r\n" line for
/// the current port. Nothing is saved: the plan never sends SAVECONFIG or FRESET.
namespace Unicore::Plan {

/// Per command.
inline constexpr std::chrono::milliseconds COMMAND_TIMEOUT{1500};

/// What completes a command: its "$command,<text>,response: OK" acknowledgement, or the log it queries. Any other
/// response status rejects the command.
enum class Reply : uint8_t
{
    Acknowledgement,
    Version,        ///< #VERSIONA names a UM980 from R4.10Build7923 or a UM982 from R4.10Build7650 (section 3.6).
    RoverMode,      ///< #MODE reports MODE ROVER.
    AveragingMode,  ///< #MODE reports MODE BASE TIME.
    FixedMode,      ///< #MODE reports MODE BASE.
    FixedPosition,  ///< #BESTNAVXYZA reports FIXEDPOS at the commanded coordinates.
};

struct Command
{
    QByteArray text;
    Reply reply = Reply::Acknowledgement;
    /// Names the command in evidence, error details and logs when its text carries base coordinates.
    QByteArray label{};
};

/// For the whole configuration after validation.
inline constexpr std::chrono::milliseconds CONFIGURATION_TIMEOUT{45000};
/// Rates the identity query probes, in order.
inline constexpr std::array BAUD_RATES{115200u, 230400u, 460800u, 921600u, 57600u, 38400u, 19200u, 9600u};

/// Sent at each candidate rate. Nothing changes before a supported receiver answers.
[[nodiscard]] Command identify();

/// The commands after identification.
struct BaseStation
{
    /// Stops logs, forces a fresh role transition through the rover role, enables NMEA output and sets the base mode.
    QList<Command> role;
    /// Sent once base status is monitored: the fixed-position readback, then base status and RTCM3 output.
    QList<Command> output;
};

/// Receiver-managed averaging for at most @a maximum.
[[nodiscard]] BaseStation averagingBase(std::chrono::seconds maximum);

/// Decimals of the ECEF coordinates fixedBase() sends and the receiver reads back: 0.1 mm.
inline constexpr int ECEF_DECIMALS = 4;
/// How far a position the receiver reports may lie from the commanded one and still be the same. It allows for the
/// rounding to ECEF_DECIMALS with a wide margin; it is not survey accuracy.
inline constexpr double SAME_POSITION_TOLERANCE_METERS = 0.02;

/// A fixed base at @a position, in ECEF metres.
[[nodiscard]] BaseStation fixedBase(const GPSProtocolMath::Ecef& position);

}  // namespace Unicore::Plan
