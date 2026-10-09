#pragma once

#include <array>
#include <chrono>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>

#include "GPSCommand.h"
#include "GPSReceiverConfig.h"

/// Septentrio base-station configuration as command lists, in the order the receiver gets them. The receiver echoes an
/// accepted command as "$R: <command>" and answers a rejected one with "$R?"; each attempt waits up to one second.
/// Commands address the connection descriptor the receiver reported, such as USB1, COM1 or IP10.
namespace SBF::Plan {

/// The link rate unless one is selected or detected. It is set on the host side first.
inline constexpr unsigned BAUD_RATE = 115200;
/// The only rate receiver detection probes.
inline constexpr std::array BAUD_RATES{BAUD_RATE};

/// Forces the connection back to command input, whatever it was set to accept.
inline constexpr QByteArrayView FORCE_COMMAND_INPUT("SSSSSSSSSS\n");

/// Asks for the command prompt, which starts with the connection descriptor, such as "USB1>".
inline constexpr QByteArrayView PROMPT("\n\r");
inline constexpr std::chrono::milliseconds PROMPT_TIMEOUT{1000};

/// Stops correction output on every connection, so the prompt can be read. Receivers without some of these
/// connections reject them, so each is optional.
[[nodiscard]] GPSCommandSequence silenceCorrectionOutput();

/// Clears the SBF output of @a port, keeps a serial port at the link's @a baud, and selects SBF and the WGS84 datum.
[[nodiscard]] GPSCommandSequence port(QByteArrayView port, unsigned baud);

/// Streams RTCM3 corrections and the PVTGeodetic survey status over @a port in the mode @a base selects.
[[nodiscard]] GPSCommandSequence base(QByteArrayView port, const GPSBaseStationConfig& base);

}  // namespace SBF::Plan
