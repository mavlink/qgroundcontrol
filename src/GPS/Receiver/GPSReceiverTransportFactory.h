#pragma once

#include "GPSProvider.h"
#include "GPSReceiverProfile.h"

namespace GPSReceiverTransportFactory {
/// Construct network I/O only when invoked by the receiver worker.
GPSProvider::TransportFactory network(const GPSReceiverProfile& profile);
}  // namespace GPSReceiverTransportFactory
