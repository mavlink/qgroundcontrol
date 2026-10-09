#pragma once

#include <span>

#include <QtCore/QLoggingCategory>

#include "GPSFamilyProtocol.h"
#include "GPSType.h"

Q_DECLARE_LOGGING_CATEGORY(AshtechProtocolLog)
Q_DECLARE_LOGGING_CATEGORY(FemtoProtocolLog)
Q_DECLARE_LOGGING_CATEGORY(PassiveProtocolLog)
Q_DECLARE_LOGGING_CATEGORY(QuectelProtocolLog)
Q_DECLARE_LOGGING_CATEGORY(SBFProtocolLog)
Q_DECLARE_LOGGING_CATEGORY(UBXProtocolLog)
Q_DECLARE_LOGGING_CATEGORY(UnicoreProtocolLog)

/// Ashtech/Trimble receivers: proprietary $PASH commands and $PASHR,POS positions over the shared NMEA and RTCM3
/// stream. Only an MB-Two can run as an RTK base station.
namespace Ashtech {
extern const GPSReceiverFamily FAMILY;
}  // namespace Ashtech

/// Femtomes receivers: ASCII commands, with GGA sentences and RTCM3 output, set up as RTK base stations.
namespace Femto {
extern const GPSReceiverFamily FAMILY;
}  // namespace Femto

/// Read-only input from a receiver configured elsewhere: standard NMEA, u-blox UBX or Septentrio SBF, whichever the
/// receiver sends, and RTCM3. Configuration only sets the local link's baud rate and never sends a receiver command.
namespace Passive {
extern const GPSReceiverFamily FAMILY;
}  // namespace Passive

/// Quectel LG290P(03) receivers configured as RTK bases with PQTM commands, independently implemented from Quectel's
/// GNSS Protocol Specification V1.1. Role and base changes need explicit per-connection consent to save and restart;
/// without it, the receiver must already have the requested settings saved.
namespace Quectel {
extern const GPSReceiverFamily FAMILY;
}  // namespace Quectel

/// Septentrio receivers configured as RTK bases with SBF commands; they stream PVTGeodetic blocks and RTCM3.
namespace SBF {
extern const GPSReceiverFamily FAMILY;
}  // namespace SBF

/// u-blox receivers over UBX: CFG-VALSET configuration from protocol 27, CFG-MSG and CFG-TMODE3 before it.
namespace UBX {
extern const GPSReceiverFamily FAMILY;
}  // namespace UBX

/// UM980/UM982 receivers with R4.10 firmware, configured as RTK bases with native N4 ASCII commands. Averaging has a
/// maximum time, not an accuracy target, and completes only on the receiver's FIXEDPOS evidence. Commands affect the
/// current port and are never saved to flash. Not hardware-qualified.
namespace Unicore {
extern const GPSReceiverFamily FAMILY;
}  // namespace Unicore

/// Every native receiver family, in lookup order.
[[nodiscard]] std::span<const GPSReceiverFamily* const> gpsReceiverFamilies();

/// The family for @a type, or nullptr when no native family supports it.
[[nodiscard]] const GPSReceiverFamily* gpsReceiverFamily(GPSType type);
