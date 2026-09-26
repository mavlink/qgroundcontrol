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

    [[nodiscard]] QByteArray name() const { return label.isEmpty() ? text : label; }
};

/// Per command.
inline constexpr std::chrono::milliseconds COMMAND_TIMEOUT{1500};
/// For the whole configuration after validation.
inline constexpr std::chrono::milliseconds CONFIGURATION_TIMEOUT{45000};
/// Rates the identity query probes, in order.
inline constexpr std::array<unsigned, 8> BAUD_RATES{115200, 230400, 460800, 921600, 57600, 38400, 19200, 9600};

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

/// A fixed base at @a position, in ECEF metres.
[[nodiscard]] BaseStation fixedBase(const GPSProtocolMath::Ecef& position);

}  // namespace Unicore::Plan
