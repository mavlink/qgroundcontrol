#pragma once

#include "GPSReceiverFamily.h"

/// Read-only NMEA and RTCM3 input from a receiver configured elsewhere. Configuration only sets the local link's baud
/// rate and never sends a receiver command.
namespace Passive {

extern const GPSReceiverFamily FAMILY;

}  // namespace Passive
