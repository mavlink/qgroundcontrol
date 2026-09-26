#pragma once

#include "GPSReceiverFamily.h"

/// UM980/UM982 receivers with R4.10 firmware, configured as RTK bases with native N4 ASCII commands. Averaging has a
/// maximum time, not an accuracy target, and completes only on the receiver's FIXEDPOS evidence. Commands affect the
/// current port and are never saved to flash. Not hardware-qualified.
namespace Unicore {

extern const GPSReceiverFamily FAMILY;

}  // namespace Unicore
