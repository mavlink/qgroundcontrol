#pragma once

#include <QtCore/QLoggingCategory>

#include "GPSReceiverFamily.h"
#include "GPSTask.h"

class GPSCommandChannel;

Q_DECLARE_LOGGING_CATEGORY(QuectelProtocolLog)

namespace Quectel {

class Decoder;

/// Configures an LG290P(03) as an RTK base: finds it at one of Plan::BAUD_RATES, or at @a baud when nonzero, which
/// then reports the rate found. The receiver must run the requested role and base from its saved settings, verified
/// after a restart; with consent to persistent changes, what differs is written, saved and restarted. Then the base
/// and NMEA output of Plan::BASE_OUTPUT and Plan::NMEA_OUTPUT is set and read back. @a decoder watches the restarts
/// and the survey. A failure leaves a description in the channel's error detail, including whether a flash save may
/// have changed the receiver. @return true when the receiver is verified as the requested base.
[[nodiscard]] GPSTask<bool> configureBase(GPSCommandChannel& channel, Decoder& decoder, GPSConfig config,
                                          unsigned& baud);

}  // namespace Quectel
