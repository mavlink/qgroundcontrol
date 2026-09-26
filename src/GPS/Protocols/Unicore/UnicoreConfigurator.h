#pragma once

#include <QtCore/QLoggingCategory>

#include "GPSReceiverFamily.h"
#include "GPSTask.h"

class GPSCommandChannel;

Q_DECLARE_LOGGING_CATEGORY(UnicoreProtocolLog)

namespace Unicore {

class Decoder;

/// Configures a UM980/UM982 as an RTK base: finds it at one of Plan::BAUD_RATES, or at @a baud when nonzero, which
/// then reports the rate found, and runs Plan::averagingBase() or Plan::fixedBase(). @a decoder verifies each reply.
/// Receiver-managed averaging is the only survey mode, and persistent changes are not supported. A failure leaves a
/// description in the channel's error detail. @return true when every command succeeded.
[[nodiscard]] GPSTask<bool> configureBase(GPSCommandChannel& channel, Decoder& decoder, GPSConfig config,
                                          unsigned& baud);

}  // namespace Unicore
