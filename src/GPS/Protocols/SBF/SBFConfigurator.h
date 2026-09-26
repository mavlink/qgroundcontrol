#pragma once

#include "GPSBaseStationConfig.h"
#include "GPSTask.h"

class GPSCommandChannel;

namespace SBF {

/// Configures a Septentrio receiver as an RTK base over the connection QGC reaches it through. The link runs at
/// Plan::BAUD_RATE, which @a baud reports. With command input forced and correction output silenced, the command
/// prompt names the connection, which the port and base plans then address; RTCM3 framing starts once that port is
/// set to stream it. @return true when the receiver accepted every required command.
[[nodiscard]] GPSTask<bool> configureBase(GPSCommandChannel& channel, GPSBaseStationConfig base, unsigned& baud);

}  // namespace SBF
