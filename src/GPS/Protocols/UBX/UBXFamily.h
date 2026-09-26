#pragma once

#include <QtCore/QLoggingCategory>

#include "GPSReceiverFamily.h"

Q_DECLARE_LOGGING_CATEGORY(UBXProtocolLog)

class GPSFamilyProtocol;
class UBXDecoder;

/// u-blox receivers over UBX: CFG-VALSET configuration from protocol 27, CFG-MSG and CFG-TMODE3 before it.
namespace UBX {

extern const GPSReceiverFamily FAMILY;

/// The decoder of @a protocol, which FAMILY created; for tests that inspect decoding state.
[[nodiscard]] UBXDecoder& decoder(GPSFamilyProtocol& protocol);

}  // namespace UBX
