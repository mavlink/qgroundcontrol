#pragma once

#include <memory>

#include "GPSDriver.h"
#include "GPSReplayTransport.h"

/// Construct the production driver from a complete captured configuration, borrowing the replay transport.
/// The caller must retain transport and its clock until the returned driver has been destroyed.
std::unique_ptr<GPSDriver> createGPSReplayDriver(GPSReplayTransport& transport, GPSDriverSinks sinks, QString& error);
