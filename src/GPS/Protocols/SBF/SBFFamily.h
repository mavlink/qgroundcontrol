#pragma once

#include <QtCore/QLoggingCategory>

#include "GPSReceiverFamily.h"

Q_DECLARE_LOGGING_CATEGORY(SBFProtocolLog)

/// Septentrio receivers configured as RTK bases with SBF commands; they stream PVTGeodetic blocks and RTCM3.
namespace SBF {

extern const GPSReceiverFamily FAMILY;

}  // namespace SBF
