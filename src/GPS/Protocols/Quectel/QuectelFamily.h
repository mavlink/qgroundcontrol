#pragma once

#include "GPSReceiverFamily.h"

/// Quectel LG290P(03) receivers configured as RTK bases with PQTM commands, independently implemented from Quectel's
/// GNSS Protocol Specification V1.1. Role and base changes need explicit per-connection consent to save and restart;
/// without it, the receiver must already have the requested settings saved.
namespace Quectel {

extern const GPSReceiverFamily FAMILY;

}  // namespace Quectel
